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
void send_client_port(const i32 clientfd, const enum operation op)
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

  if (op != METADATA && IsFile(NM_Tree, path) == 0)
  {
    code = INVALID_TYPE;
    LOG("Can't do operation %d on directory %s\n", op, path);
    LOG_SEND(clientfd, code);
    return;
  }

  if (op == READ || op == METADATA)
    AcquireReaderLock(NM_Tree, path);
  else
    AcquireWriterLock(NM_Tree, path);

  LOG("Found storage server client port %i for path %s\n", port, path);
  LOG_SEND(clientfd, code);
  LOG_SEND(clientfd, port);
  enum operation ack;
  LOG_RECV(clientfd, ack);

  ReleaseLock(NM_Tree, path);
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
  char *parent = GetParent(path);
  if (parent == NULL)
  {
    temp = MinSizeStorageServer();
  }
  else
  {
    temp = ss_from_path(parent, true);
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
  RECV(clientfd, path);

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

  bool is_file = IsFile(NM_Tree, path);
  if ((op == DELETE_FILE && !is_file) || (op == DELETE_FOLDER && is_file))
  {
    code = INVALID_TYPE;
    LOG_SEND(clientfd, code);
    return;
  }

  LOG("Found storage server - naming server port %i corresponding to the path %s\n", port, path);
  const i32 sockfd = connect_to_port(port);
  SEND(sockfd, op);
  SEND(sockfd, path);

  AcquireWriterLock(NM_Tree, path);

  // send status code received from ss to client
  RECV(sockfd, code);
  SEND(clientfd, code);
  close(sockfd);

  if (code != SUCCESS)
  {
    ReleaseLock(NM_Tree, path);
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

/**
 * @brief Copy a file or folder across different storage servers or same storage servers
 *
 * @param CopyTree The tree being copied from
 * @param from_path current file/folder path being copied
 * @param dest_path destination path
 * @param from_sockfd socket of the storage server being copied from
 * @param to_sockfd socket of the storage server being copied to
 * @param to_port nm port for the `to` storage server
 * @param UUID unique identifier of the `to` storage server
 */
void copy_file_or_folder(Tree CopyTree, const char *from_path, const char *dest_path, const i32 from_sockfd,
                         const i32 to_sockfd, const i32 to_port, char *UUID)
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
void copy_operation(const i32 clientfd, const enum operation op)
{
  enum status code = SUCCESS;

  char from_path[MAX_STR_LEN];
  char to_path[MAX_STR_LEN];
  RECV(clientfd, from_path);
  RECV(clientfd, to_path);
  bool cache_flag = true;
  if (strncmp(from_path, ".rd", 3) * strncmp(to_path, ".rd", 3) == 0)
    cache_flag = false;

  LOG("Finding storage server - naming server port corresponding to path %s\n", from_path);
  storage_server_data *from_ss = ss_from_path(from_path, cache_flag);

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
  storage_server_data *to_ss = ss_from_path(to_path, cache_flag);
  if (to_ss == NULL)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", to_path);
    code = NOT_FOUND;
    LOG_SEND(clientfd, code);
    return;
  }
  const i32 to_port = to_ss->port_for_nm;

  if (Ancestor(NM_Tree, from_path, to_path))
  {
    LOG("from_path Ancestor of to_path - naming server port corresponding to the path %s\n", to_path);
    code = RECURSIVE_COPY;
    LOG_SEND(clientfd, code);
    return;
  }

  if ((op == COPY_FILE && !IsFile(NM_Tree, from_path)) || (op == COPY_FOLDER && IsFile(NM_Tree, from_path)))
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", from_path);
    code = INVALID_TYPE;
    LOG_SEND(clientfd, code);
    return;
  }
  LOG("Found storage server - naming server port corresponding to path %s\n", to_path);

  AcquireReaderLock(NM_Tree, from_path);

  Tree CopyTree = GetTreeFromPath(NM_Tree, from_path);
  strcat(to_path, "/");
  strcat(to_path, CopyTree->NodeInfo.DirectoryName);

  if (IsFile(NM_Tree, to_path) != -1)
  {
    LOG("File already exists - naming server port corresponding to the path %s\n", from_path);
    code = ALREADY_EXISTS;
    LOG_SEND(clientfd, code);
    return;
  }

  const i32 from_sockfd = connect_to_port(from_port);
  const i32 to_sockfd = connect_to_port(to_port);

  enum copy_type ch = SENDER;

  SEND(from_sockfd, op);
  SEND(from_sockfd, ch);

  ch = RECEIVER;

  SEND(to_sockfd, op);
  SEND(to_sockfd, ch);

  copy_file_or_folder(CopyTree, from_path, to_path, from_sockfd, to_sockfd, to_port, to_ss->UUID);

  i8 is_file = 2;
  SEND(to_sockfd, is_file);
  SEND(from_sockfd, is_file);

  RECV(from_sockfd, code);
  RECV(to_sockfd, code);

  SEND(clientfd, code);

  ReleaseLock(NM_Tree, from_path);

  close(from_sockfd);
  close(to_sockfd);
}

void send_tree_for_printing(const i32 clientfd)
{
  enum status code = SUCCESS;
  char path[MAX_STR_LEN];
  RECV(clientfd, path);
  if (IsFile(NM_Tree, path) == -1)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", path);
    code = INVALID_TYPE;
    LOG_SEND(clientfd, code);
    return;
  }
  SEND(clientfd, code);
  char printedtree[50 * MAX_STR_LEN] = {0};
  GetPrintedSubtree(NM_Tree, path, printedtree);
  SEND(clientfd, printedtree);
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
      send_client_port(clientfd, op);
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
      copy_operation(clientfd, op);
      break;
    case PRINT_TREE:
      send_tree_for_printing(clientfd);
      break;
    case DISCONNECT:
      disconnect = true;
      LOG("Client disconnected\n");
      break;
    default:
      disconnect = true;
      LOG("Received invalid operation: %d\n", op);
      break;
    }
  }

  return NULL;
}
