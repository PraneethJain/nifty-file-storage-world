/**
 * @file nm_to_ss.c
 * @brief Communication between the naming server and storage servers
 * @details
 * - Stores all the currently connected storage servers
 * - Manages adding and removing these servers whenever needed
 * - Retrieves storage server data from paths provided
 */

#include "../common/headers.h"
#include "headers.h"

typedef struct connected_storage_server_node
{
  storage_server_data data;
  struct connected_storage_server_node *next;
} connected_storage_server_node;

struct
{
  u32 length;
  connected_storage_server_node *first;
} connected_storage_servers = {0};

/**
 * @brief Initialize a new storage server node with data
 *
 * @param data
 * @return connected_storage_server_node*
 */
connected_storage_server_node *init_connected_storage_server_node(storage_server_data data)
{
  connected_storage_server_node *n = malloc(sizeof(connected_storage_server_node));
  n->data = data;
  n->next = NULL;

  return n;
}

/**
 * @brief Add a connected storage server to the linked list and merge accessible paths
 *
 * @param data
 */
void add_connected_storage_server(storage_server_data data)
{
  if (connected_storage_servers.length == 0)
  {
    connected_storage_servers.first = init_connected_storage_server_node(data);
  }
  else
  {
    connected_storage_server_node *cur;
    for (cur = connected_storage_servers.first; cur->next != NULL; cur = cur->next)
      ;
    cur->next = init_connected_storage_server_node(data);
  }

  ++connected_storage_servers.length;

  Tree temp = ReceiveTreeData(data.ss_tree);
  pthread_mutex_lock(&tree_lock);
  MergeTree(NM_Tree, temp, data.port_for_nm, data.UUID);
  pthread_mutex_unlock(&tree_lock);
  PrintTree(NM_Tree, 0);
}

/**
 * @brief Receive initial port and accessible paths from all new storage servers
 *
 * @param arg NULL
 * @return void* NULL
 */
void *storage_server_init(void *arg)
{
  (void)arg;

  const i32 serverfd = bind_to_port(NM_SS_PORT);
  printf("Listening for storage servers on port %i\n", NM_SS_PORT);
  LOG("Listening for storage servers on port %i\n", NM_SS_PORT);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    LOG("Awaiting a connection on socket FD\n");
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);
    LOG("Accepted connection on socket FD\n");
    storage_server_data resp;
    receive_data_in_packets(&resp, clientfd, sizeof(resp));
    LOG("Received initial information of storage server with UUID %s\n", resp.UUID);

    CHECK(close(clientfd), -1);
    add_connected_storage_server(resp);
  }

  CHECK(close(serverfd), -1);
  return NULL;
}

/**
 * @brief Deletes and copies redundancies. Is called periodically
 *
 * @param T root node to delete redundancies of
 * @param rd_num redundancy number to copy to
 * @param nm_sockfd socket of the naming server
 * @return enum status
 */
enum status delete_and_copy(const Tree T, const i32 rd_num, const i32 nm_sockfd)
{
  char from_path[MAX_STR_LEN] = {0};
  strcpy(from_path, T->NodeInfo.DirectoryName);

  char to_path[MAX_STR_LEN] = {0};
  sprintf(to_path, ".rd%i", rd_num);

  char delete_path[MAX_STR_LEN] = {0};
  sprintf(delete_path, ".rd%i/%s", rd_num, T->NodeInfo.DirectoryName);

  enum operation op = T->NodeInfo.IsFile ? DELETE_FILE : DELETE_FOLDER;
  SEND(nm_sockfd, op);
  SEND(nm_sockfd, delete_path);
  enum status code;
  RECV(nm_sockfd, code);
  if (code != SUCCESS && code != NOT_FOUND)
    return code;

  op = T->NodeInfo.IsFile ? COPY_FILE : COPY_FOLDER;
  SEND(nm_sockfd, op);
  SEND(nm_sockfd, from_path);
  SEND(nm_sockfd, to_path);
  RECV(nm_sockfd, code);
  return code;
}

/**
 * @brief Ensure redundancy of every tree storage server in two others at all times
 * Update old copies with new updated ones.
 *
 * @param nm_sockfd socket of the naming server
 */
void issue_redundancy_commands(const i32 nm_sockfd)
{
  if (connected_storage_servers.length < 3)
    return;

  for (Tree T = NM_Tree->ChildDirectoryLL; T != NULL; T = T->NextSibling)
  {
    if (strcmp(T->NodeInfo.UUID, RD1) == 0)
    {
      if (strstr(T->NodeInfo.DirectoryName, ".rd1") == T->NodeInfo.DirectoryName)
        continue;
      delete_and_copy(T, 2, nm_sockfd);
      delete_and_copy(T, 3, nm_sockfd);
    }
    else if (strcmp(T->NodeInfo.UUID, RD2) == 0)
    {
      if (strstr(T->NodeInfo.DirectoryName, ".rd2") == T->NodeInfo.DirectoryName)
        continue;
      delete_and_copy(T, 1, nm_sockfd);
      delete_and_copy(T, 3, nm_sockfd);
    }
    else
    {
      if (strstr(T->NodeInfo.DirectoryName, ".rd3") == T->NodeInfo.DirectoryName)
        continue;
      delete_and_copy(T, 1, nm_sockfd);
      delete_and_copy(T, 2, nm_sockfd);
    }
  }
}

/**
 * @brief Periodically check if each storage server is still alive.
 * Disconnect the ones that have crashed.
 * Issue redundant delete and copy commands.
 *
 * @param arg NULL
 * @return void* NULL
 */
void *alive_checker(void *arg)
{
  (void)arg;
  sleep(5);
  const i32 nm_sockfd = connect_to_port(NM_CLIENT_PORT);
  while (1)
  {
    sleep(15);
    connected_storage_server_node *cur = connected_storage_servers.first;
    connected_storage_server_node *prev = NULL;
    while (cur != NULL)
    {
      const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
      CHECK(sockfd, -1);
      struct sockaddr_in addr;
      memset(&addr, '\0', sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(cur->data.port_for_alive);
      addr.sin_addr.s_addr = inet_addr(LOCALHOST);
      if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
      {
        if (errno == 111) // Connection refused
        {
          printf("Storage server with ssid %i has disconnected!\n", cur->data.port_for_nm);
          LOG("Storage server with ssid %i disconnected\n", cur->data.port_for_nm);

          pthread_mutex_lock(&tree_lock);
          RemoveServerPath(NM_Tree, cur->data.port_for_nm);
          pthread_mutex_unlock(&tree_lock);

          if (prev == NULL)
          {
            connected_storage_servers.first = cur->next;
          }
          else
          {
            prev->next = cur->next;
          }
          connected_storage_server_node *temp = cur->next;
          free(cur);
          cur = temp;
          --connected_storage_servers.length;
          CHECK(close(sockfd), -1);
          continue;
        }
        else
        {
          ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));
          exit(1);
        }
      }
      else
      {
        // ok
      }

      CHECK(close(sockfd), -1);

      prev = cur;
      cur = cur->next;
    }

    issue_redundancy_commands(nm_sockfd);
  }
  close(nm_sockfd);
  return NULL;
}

/**
 * @brief Find the minimum size storage server and return it.
 *
 * @return storage_server_data*
 */
storage_server_data *MinSizeStorageServer()
{
  connected_storage_server_node *BestSS = connected_storage_servers.first;
  size_t minsize = strlen(connected_storage_servers.first->data.ss_tree);
  for (connected_storage_server_node *cur = connected_storage_servers.first; cur != NULL; cur = cur->next)
  {
    if (strlen(cur->data.ss_tree) < minsize)
    {
      minsize = strlen(cur->data.ss_tree);
      BestSS = cur;
    }
  }
  if (BestSS == NULL)
    return NULL;
  else
    return &BestSS->data;
}

/**
 * @brief Finds the storage server corresponding to the path and returns its data
 *
 * @param path
 * @param cache_flag
 * @return storage_server_data
 */
storage_server_data *ss_from_path(const char *path, bool cache_flag)
{
  cache_flag &= strncmp(path, ".rd", 3) != 0;
  i32 ssid = GetPathSSID(NM_Tree, path, cache_flag);
  if (ssid != -1)
  {
    for (connected_storage_server_node *cur = connected_storage_servers.first; cur != NULL; cur = cur->next)
    {
      if (cur->data.port_for_nm == ssid)
      {
        return &cur->data;
      }
    }
  }
  return NULL;
}

/**
 * @brief Finds the storage server client port corresponding to the path
 *
 * @param path
 * @return i32 storage server client port
 */
i32 ss_client_port_from_path(const char *path)
{
  storage_server_data *ss_info = ss_from_path(path, true);
  if (ss_info == NULL)
  {
    return -1;
  }
  return ss_info->port_for_client;
}

/**
 * @brief Finds the storage server nm port corresponding to the path
 *
 * @param path
 * @return i32 storage server nm port
 */
i32 ss_nm_port_from_path(const char *path)
{
  storage_server_data *ss_info = ss_from_path(path, true);
  if (ss_info == NULL)
  {
    return -1;
  }
  return ss_info->port_for_nm;
}

/**
 * @brief Finds the storage server nm port of minimum size
 *
 * @return i32
 */
i32 ss_nm_port_new()
{
  storage_server_data *new_ss = MinSizeStorageServer();
  if (new_ss == NULL)
    return -1;
  return new_ss->port_for_nm;
}
