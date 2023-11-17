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

extern Tree NM_Tree;

/**
 * @brief Receive path from client and send the corresponding storage server's port
 *
 * @param clientfd file descriptor of the client socket
 */
void send_client_port(const i32 clientfd)
{
  char path[MAX_STR_LEN];

  LOG("Receiving path from client at port %i\n", NM_CLIENT_PORT);
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);
  LOG("Received path %s from client at port %i\n", path, NM_CLIENT_PORT);
  LOG("Finding storage server client port for path %s\n", path);
  const i32 port = ss_client_port_from_path(path);
  enum status code = SUCCESS;

  if (port == -1)
  {
    code = NOT_FOUND;
    LOG("Not found storage server client port for path %s\n", path);
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    return;
  }
  LOG("Found storage server client port %i for path %s\n", port, path);
  LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
  CHECK(send(clientfd, &code, sizeof(code), 0), -1);
  LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
  LOG("Sending port %i to client at port %i\n", port, NM_CLIENT_PORT);
  CHECK(send(clientfd, &port, sizeof(port), 0), -1);
  LOG("Sent port %i to client at port %i\n", port, NM_CLIENT_PORT);
}

/**
 * @brief Receive path from client, perform specified operation on storage server and send the status code
 *
 * @param clientfd file descriptor of the client socket
 * @param op specified operation
 */
void send_nm_op_single(const i32 clientfd, const enum operation op)
{
  char path[MAX_STR_LEN];
  LOG("Receiving path from client at port %i\n", NM_CLIENT_PORT);
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);
  LOG("Received path %s from client at port %i\n", path, NM_CLIENT_PORT);

  enum status code;
  char *parent = get_parent(path);
  i32 port;
  LOG("Finding storage server - naming server port corresponding to the path %s\n", path);
  if (parent == NULL)
  {
    port = ss_nm_port_new();
  }
  else
  {
    port = ss_nm_port_from_path(parent);
    free(parent);
  }
  if (port == -1)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", path);
    code = NOT_FOUND;
    LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
    return;
  }
  LOG("Found storage server - naming server port %i corresponding to the path %s\n", port, path);
  const i32 sockfd = connect_to_port(port);
  LOG("Sending operation %i to storage server at port %i\n", op, NM_SS_PORT);
  CHECK(send(sockfd, &op, sizeof(op), 0), -1);
  LOG("Sent operation %i to storage server at port %i\n", op, NM_SS_PORT);
  LOG("Sending path %s to storage server at port %i\n", path, NM_SS_PORT);
  CHECK(send(sockfd, path, sizeof(path), 0), -1);
  LOG("Sent path %s to storage server at port %i\n", path, NM_SS_PORT);

  // send status code received from ss to client
  LOG("Receiving code %i from storage server at port %i\n", op, NM_SS_PORT);
  CHECK(recv(sockfd, &code, sizeof(code), 0), -1);
  LOG("Received code %i to storage server at port %i\n", op, NM_SS_PORT);
  LOG("Sending code %i to client at port %i\n", op, NM_CLIENT_PORT);
  CHECK(send(clientfd, &code, sizeof(code), 0), -1);
  LOG("Sent code %i to client at port %i\n", op, NM_CLIENT_PORT);
  close(sockfd);

  if (code != SUCCESS)
  {
    LOG("Operation failed with code %i\n", code);
    return;
  }
  if (op == CREATE_FILE)
  {
    AddFile(NM_Tree, path, port);
    LOG("Added file %s to NM Tree\n", path);
  }
  else if (op == DELETE_FILE)
  {
    DeleteFile(NM_Tree, path);
    LOG("Deleted file %s from NM Tree\n", path);
  }
  else if (op == CREATE_FOLDER)
  {
    AddFolder(NM_Tree, path, port);
    LOG("Added folder %s to NM Tree\n", path);
  }
  else if (op == DELETE_FOLDER)
  {
    DeleteFolder(NM_Tree, path);
    LOG("Deleted folder %s from NM Tree\n", path);
  }
}

void copy_file_or_folder(Tree CopyTree, char *from_path, char *dest_path, const i32 from_sockfd, const i32 to_sockfd,
                         const i32 to_port)
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

    AddFile(NM_Tree, dest_path, to_port);
    return;
  }
  else
  {
    CHECK(recv(to_sockfd, &code, sizeof(code), 0), -1);
    AddFolder(NM_Tree, dest_path, to_port);
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

    copy_file_or_folder(trav, from_path_copy, to_path_copy, from_sockfd, to_sockfd, to_port);
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
  LOG("Receiving copy source path from client at port %i\n", NM_CLIENT_PORT);
  CHECK(recv(clientfd, from_path, sizeof(from_path), 0), -1);
  CHECK(send(clientfd, &code, sizeof(code), 0), -1);
  LOG("Received copy source path %s from client at port %i\n", from_path, NM_CLIENT_PORT);

  LOG("Receiving copy destination path from client at port %i\n", NM_CLIENT_PORT);
  CHECK(recv(clientfd, to_path, sizeof(to_path), 0), -1);
  CHECK(send(clientfd, &code, sizeof(code), 0), -1);
  LOG("Received copy destination path %s from client at port %i\n", to_path, NM_CLIENT_PORT);
  LOG("Finding storage server - naming server port corresponding to path %s\n", from_path);
  const i32 from_port = ss_nm_port_from_path(from_path);
  LOG("Found storage server - naming server port corresponding to path %s\n", from_path);
  LOG("Finding storage server - naming server port corresponding to path %s\n", to_path);
  const i32 to_port = ss_nm_port_from_path(to_path);

  if (from_port == -1)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", from_path);
    code = NOT_FOUND;
    LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
    return;
  }

  if (to_port == -1)
  {
    LOG("Not found storage server - naming server port corresponding to the path %s\n", to_path);
    code = NOT_FOUND;
    LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
    return;
  }

  if (ancestor(NM_Tree, from_path, to_path))
  {
    LOG("from_path ancestor of to_path - naming server port corresponding to the path %s\n", to_path);
    code = UNKNOWN_PERMISSION_DENIED;
    LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
    return;
  }

  if (op == COPY_FOLDER)
  {
    if (Is_File(NM_Tree, from_path))
    {
      LOG("Not found storage server - naming server port corresponding to the path %s\n", from_path);
      code = NOT_FOUND;
      LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
      CHECK(send(clientfd, &code, sizeof(code), 0), -1);
      LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
      return;
    }
  }
  else if (op == COPY_FILE)
  {
    if (!Is_File(NM_Tree, from_path))
    {
      LOG("Not found storage server - naming server port corresponding to the path %s\n", from_path);
      code = NOT_FOUND;
      LOG("Sending code %i to client at port %i\n", code, NM_CLIENT_PORT);
      CHECK(send(clientfd, &code, sizeof(code), 0), -1);
      LOG("Sent code %i to client at port %i\n", code, NM_CLIENT_PORT);
      return;
    }
  }

  Tree CopyTree = GetTreeFromPath(NM_Tree, from_path);
  strcat(to_path, "/");
  strcat(to_path, CopyTree->NodeInfo.DirectoryName);

  LOG("Found storage server - naming server port corresponding to path %s\n", to_path);
  const i32 from_sockfd = connect_to_port(from_port);
  const i32 to_sockfd = connect_to_port(to_port);

  enum copy_type ch = SENDER;

  CHECK(send(from_sockfd, &op, sizeof(op), 0), -1);
  CHECK(send(from_sockfd, &ch, sizeof(ch), 0), -1);

  ch = RECEIVER;

  CHECK(send(to_sockfd, &op, sizeof(op), 0), -1);
  CHECK(send(to_sockfd, &ch, sizeof(ch), 0), -1);

  copy_file_or_folder(CopyTree, from_path, to_path, from_sockfd, to_sockfd, to_port);

  i8 is_file = 2;
  CHECK(send(to_sockfd, &is_file, sizeof(is_file), 0), -1);
  CHECK(send(from_sockfd, &is_file, sizeof(is_file), 0), -1);

  CHECK(recv(from_sockfd, &code, sizeof(code), 0), -1);
  CHECK(recv(to_sockfd, &code, sizeof(code), 0), -1);

  CHECK(send(clientfd, &code, sizeof(code), 0), -1);

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
    LOG("Receiving operation from client at port %i\n", NM_CLIENT_PORT);
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1)
    LOG("Received operation %i from client at port %i\n", op, NM_CLIENT_PORT);
    switch (op)
    {
    case READ:
    case WRITE:
    case METADATA:
      send_client_port(clientfd);
      break;
    case CREATE_FILE:
    case DELETE_FILE:
    case CREATE_FOLDER:
    case DELETE_FOLDER:
      send_nm_op_single(clientfd, op);
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
