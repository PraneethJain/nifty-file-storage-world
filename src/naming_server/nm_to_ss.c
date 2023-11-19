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

extern Tree NM_Tree;

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
 * @brief Add a connected storage server to the linked list
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
  MergeTree(NM_Tree, temp, data.port_for_nm, data.UUID);
  PrintTree(NM_Tree, 0);
}

/**
 * @brief Receive initial port and directory information from all new storage servers
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
    LOG_RECV(clientfd, resp);
    printf("%i %i %i\n", resp.port_for_client, resp.port_for_nm, resp.port_for_alive);

    CHECK(close(clientfd), -1);
    add_connected_storage_server(resp);
  }

  CHECK(close(serverfd), -1);
  return NULL;
}

/**
 * @brief Periodically check if each storage server is still alive.
 * Disconnect the ones that have crashed.
 *
 * @param arg NULL
 * @return void* NULL
 */
void *alive_checker(void *arg)
{
  (void)arg;
  while (1)
  {
    sleep(5);
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
          RemoveServerPath(NM_Tree, cur->data.port_for_nm);
          if (prev == NULL)
          {
            connected_storage_servers.first = cur->next;
          }
          else
          {
            prev->next = cur->next;
          }
          free(cur);
          --connected_storage_servers.length;
          break;
        }
        else
        {
          ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));
          exit(1);
        }
      }
      else if (connected_storage_servers.length >= 3)
      {
        // if num storage servers >= 3
        // delete .rd1/cur
        // copy over cur to .rd1/cur
        // same with .rd2

        connected_storage_server_node *cur_red_ss = connected_storage_servers.first;
        for (int i=0; i<3; cur_red_ss = cur_red_ss->next, i++)
        {
          if (cur_red_ss->data.UUID == cur->data.UUID)
            continue;

          for (Tree T = NM_Tree->ChildDirectoryLL; T != NULL; T = T->NextSibling)
          {
            // if ()
            // Delete Directory -> cur_red_ss name/.rdi/T->Name

            // const i32 from_sockfd = connect_to_port(from_port);
            // const i32 to_sockfd = connect_to_port(to_port);
          }
        }

        
      }

      CHECK(close(sockfd), -1);

      prev = cur;
      cur = cur->next;
    }
  }
  return NULL;
}

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
 * @return storage_server_data
 */
storage_server_data *ss_from_path(const char *path)
{
  i32 ssid = GetPathSSID(NM_Tree, path);
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
  storage_server_data *ss_info = ss_from_path(path);
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
  storage_server_data *ss_info = ss_from_path(path);
  if (ss_info == NULL)
  {
    return -1;
  }
  return ss_info->port_for_nm;
}

i32 ss_nm_port_new()
{
  storage_server_data *new_ss = MinSizeStorageServer();
  if (new_ss == NULL)
    return -1;
  return new_ss->port_for_nm;
}
