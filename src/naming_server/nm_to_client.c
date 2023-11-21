/**
 * @file nm_to_client.c
 * @brief Communication between the naming server and clients
 * @details
 * - Receives initial client connections and spawns a new thread for each client
 * - Handles all operations sent to the naming server from the client
 * - Forwards requests to the storage server whenever needed
 */

#include "../common/headers.h"
#include "headers.h"

/**
 * @brief Receive path from client and send the corresponding storage server's port
 *
 * @param clientfd file descriptor of the client socket
 */
void send_client_port(const i32 clientfd)
{
  char path[MAX_STR_LEN];

  LOG_RECV(clientfd, path);
  LOG("Finding storage server client port for path %s\n", path);
  const i32 port = ss_client_port_from_path(path);
  enum status code = SUCCESS;

  if (port == -1)
  {
    code = NOT_FOUND;
    LOG("Not found storage server client port for path %s\n", path);
    LOG_SEND(clientfd, code);
    return;
  }
  LOG("Found storage server client port %i for path %s\n", port, path);
  LOG_SEND(clientfd, code);
  LOG_SEND(clientfd, port);
}

/**
 * @brief Receive path from client, perform create operation on storage server and send the status code
 *
 * @param clientfd file descriptor of the client socket
 * @param op specified operation
 */
void create_operations(const i32 clientfd, const enum operation op)
{
  char path[MAX_STR_LEN];
  LOG_RECV(clientfd, path);

  enum status code;
  i32 port;
  storage_server_data *temp = NULL;
  char *parent = get_parent(path);
  if (parent == NULL)
  {
    temp = MinSizeStorageServer();
  }
  else
  {
    temp = ss_from_path(parent);
    free(parent);
  }
  if (temp == NULL)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", path);
    code = NOT_FOUND;
    LOG_SEND(clientfd, code);
    return;
  }
  port = temp->port_for_nm;

  LOG("Found storage server - naming server port %i corresponding to the path %s\n", port, path);
  const i32 sockfd = connect_to_port(port);
  LOG_SEND(sockfd, op);
  LOG_SEND(sockfd, path);

  // send status code received from ss to client
  LOG_RECV(sockfd, code);
  LOG_SEND(clientfd, code);
  close(sockfd);

  if (code != SUCCESS)
  {
    LOG("Operation failed with code %i\n", code);
    return;
  }
  if (op == CREATE_FILE)
  {
    pthread_mutex_lock(&tree_lock);
    AddFile(NM_Tree, path, port, temp->UUID);
    pthread_mutex_unlock(&tree_lock);
    LOG("Added file %s to NM Tree\n", path);
  }
  else if (op == CREATE_FOLDER)
  {
    pthread_mutex_lock(&tree_lock);
    AddFolder(NM_Tree, path, port, temp->UUID);
    pthread_mutex_unlock(&tree_lock);
    LOG("Added folder %s to NM Tree\n", path);
  }
}

/**
 * @brief receive path from client, perform delete operation on storage server and send the status code
 *
 * @param clientfd file descriptor of the client socket
 * @param op specified operation
 */
void delete_operations(const i32 clientfd, const enum operation op)
{
  char path[MAX_STR_LEN];
  LOG_RECV(clientfd, path);

  enum status code;
  LOG("Finding storage server - naming server port corresponding to the path %s\n", path);
  const i32 port = ss_nm_port_from_path(path);

  if (port == -1)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", path);
    code = NOT_FOUND;
    LOG_SEND(clientfd, code);
    return;
  }

  bool is_file = Is_File(NM_Tree, path);
  if ((op == DELETE_FILE && !is_file) || (op == DELETE_FOLDER && is_file))
  {
    code = INVALID_PATH;
    LOG_SEND(clientfd, code);
    return;
  }

  LOG("Found storage server - naming server port %i corresponding to the path %s\n", port, path);
  const i32 sockfd = connect_to_port(port);
  LOG_SEND(sockfd, op);
  LOG_SEND(sockfd, path);

  // send status code received from ss to client
  LOG_RECV(sockfd, code);
  LOG_SEND(clientfd, code);
  close(sockfd);

  if (code != SUCCESS)
  {
    LOG("Operation failed with code %i\n", code);
    return;
  }

  if (op == DELETE_FILE)
  {
    pthread_mutex_lock(&tree_lock);
    DeleteFile(NM_Tree, path);
    pthread_mutex_unlock(&tree_lock);
    LOG("Deleted file %s from NM Tree\n", path);
  }
  else if (op == DELETE_FOLDER)
  {
    pthread_mutex_lock(&tree_lock);
    DeleteFolder(NM_Tree, path);
    pthread_mutex_unlock(&tree_lock);
    LOG("Deleted folder %s from NM Tree\n", path);
  }
}

void copy_file_or_folder(Tree CopyTree, char *from_path, char *dest_path, const i32 from_sockfd, const i32 to_sockfd,
                         const i32 to_port, char *UUID)
{
  enum status code;
  i8 is_file = CopyTree->NodeInfo.IsFile;
  CHECK(send(to_sockfd, &is_file, sizeof(is_file), 0), -1);
  CHECK(send(to_sockfd, dest_path, MAX_STR_LEN, 0), -1);

  if (is_file)
  {
    CHECK(send(from_sockfd, &is_file, sizeof(is_file), 0), -1);
    CHECK(send(from_sockfd, from_path, MAX_STR_LEN, 0), -1);

    CHECK(recv(from_sockfd, &code, sizeof(code), 0), -1);
    CHECK(recv(to_sockfd, &code, sizeof(code), 0), -1);

    receive_and_transmit_file(from_sockfd, to_sockfd);

    pthread_mutex_lock(&tree_lock);
    AddFile(NM_Tree, dest_path, to_port, UUID);
    pthread_mutex_unlock(&tree_lock);
    return;
  }
  else
  {
    CHECK(recv(to_sockfd, &code, sizeof(code), 0), -1);
    pthread_mutex_unlock(&tree_lock);
    AddFolder(NM_Tree, dest_path, to_port, UUID);
    pthread_mutex_unlock(&tree_lock);
  }

  for (Tree trav = CopyTree->ChildDirectoryLL; trav != NULL; trav = trav->NextSibling)
  {
    char from_path_copy[MAX_STR_LEN];
    char to_path_copy[MAX_STR_LEN];
    strcpy(from_path_copy, from_path);
    strcat(from_path_copy, "/");
    strcat(from_path_copy, trav->NodeInfo.DirectoryName);

    strcpy(to_path_copy, dest_path);
    strcat(to_path_copy, "/");
    strcat(to_path_copy, trav->NodeInfo.DirectoryName);

    copy_file_or_folder(trav, from_path_copy, to_path_copy, from_sockfd, to_sockfd, to_port, UUID);
  }
}

/**
 * @brief Receive 2 paths from client, perform copy operation on storage server and send the status code
 *
 * @param clientfd file descriptor of the client socket
 * @param op specified operation
 */
void send_nm_op_double(const i32 clientfd, const enum operation op)
{
  enum status code = SUCCESS;

  char from_path[MAX_STR_LEN];
  char to_path[MAX_STR_LEN];
  LOG_RECV(clientfd, from_path);
  LOG_RECV(clientfd, to_path);

  LOG("Finding storage server - naming server port corresponding to path %s\n", from_path);
  storage_server_data *from_ss = ss_from_path(from_path);

  if (from_ss == NULL)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", from_path);
    code = NOT_FOUND;
    LOG_SEND(clientfd, code);
    return;
  }
  LOG("Found storage server - naming server port corresponding to path %s\n", from_path);
  const i32 from_port = from_ss->port_for_nm;

  LOG("Finding storage server - naming server port corresponding to path %s\n", to_path);
  storage_server_data *to_ss = ss_from_path(to_path);
  if (to_ss == NULL)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", to_path);
    code = NOT_FOUND;
    LOG_SEND(clientfd, code);
    return;
  }
  const i32 to_port = to_ss->port_for_nm;

  if (ancestor(NM_Tree, from_path, to_path))
  {
    LOG("from_path ancestor of to_path - naming server port corresponding to the path %s\n", to_path);
    code = UNKNOWN_PERMISSION_DENIED;
    LOG_SEND(clientfd, code);
    return;
  }

  if ((op == CREATE_FILE && !Is_File(NM_Tree, from_path)) || (op == COPY_FOLDER && Is_File(NM_Tree, from_path)))
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", from_path);
    code = NOT_FOUND;
    LOG_SEND(clientfd, code);
    return;
  }
  LOG("Found storage server - naming server port corresponding to path %s\n", to_path);

  Tree CopyTree = GetTreeFromPath(NM_Tree, from_path);
  strcat(to_path, "/");
  strcat(to_path, CopyTree->NodeInfo.DirectoryName);

  const i32 from_sockfd = connect_to_port(from_port);
  const i32 to_sockfd = connect_to_port(to_port);

  enum copy_type ch = SENDER;

  LOG_SEND(from_sockfd, op);
  LOG_SEND(from_sockfd, ch);

  ch = RECEIVER;

  LOG_SEND(to_sockfd, op);
  LOG_SEND(to_sockfd, ch);

  copy_file_or_folder(CopyTree, from_path, to_path, from_sockfd, to_sockfd, to_port, to_ss->UUID);

  i8 is_file = 2;
  LOG_SEND(to_sockfd, is_file);
  LOG_SEND(from_sockfd, is_file);

  LOG_RECV(from_sockfd, code);
  LOG_RECV(to_sockfd, code);

  LOG_SEND(clientfd, code);

  close(from_sockfd);
  close(to_sockfd);
}

/**
 * @brief Initializes connection to the clients and spawns a new client relay for each of them
 *
 * @param arg NULL
 * @return void* NULL
 */
void *client_init(void *arg)
{
  (void)arg;

  const i32 serverfd = bind_to_port(NM_CLIENT_PORT);
  printf("Listening for clients on port %i\n", NM_CLIENT_PORT);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    i32 *clientfd = malloc(sizeof(i32));
    *clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(*clientfd, -1);

    pthread_t client_relay_thread;
    pthread_create(&client_relay_thread, NULL, client_relay, clientfd);
  }
  // maybe join client_relays if ever a break statement is added

  CHECK(close(serverfd), -1);

  return NULL;
}

/**
 * @brief Receives all operations from the client.
 * In case of READ, WRITE and METADATA, sends the port number of the corresponding storage server to the client.
 * In other cases, performs the operation and sends the status code to the client
 * @param arg integer pointer to the client file descriptor
 * @return void* NULL
 */
void *client_relay(void *arg)
{
  const i32 clientfd = *(i32 *)arg;
  free(arg);
  bool disconnect = false;
  while (!disconnect)
  {
    enum operation op;
    LOG_RECV(clientfd, op);
    switch (op)
    {
    case READ:
    case WRITE:
    case METADATA:
      send_client_port(clientfd);
      break;
    case CREATE_FILE:
    case CREATE_FOLDER:
      create_operations(clientfd, op);
      break;
    case DELETE_FILE:
    case DELETE_FOLDER:
      delete_operations(clientfd, op);
      break;
    case COPY_FILE:
    case COPY_FOLDER:
      send_nm_op_double(clientfd, op);
      break;
    case DISCONNECT:
      disconnect = true;
      LOG("Client disconnected\n");
      break;
    }
  }

  return NULL;
}
