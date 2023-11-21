/**
 * @file ss_to_nm.c
 * @brief Communication between a storage server and the naming server
 * @details
 * - Informs naming server upon initialization
 * - Responds to alive requests from the naming server
 * - Handles operations received from the naming server
 */

#include "../common/headers.h"
#include "headers.h"

/**
 * @brief Send ports and accessible paths to the naming server upon this storage server's initialization
 *
 * @param arg NULL
 * @return void* NULL
 */
void *init_storage_server(void *arg)
{
  (void)arg;

  sem_wait(&client_port_created);
  sem_wait(&nm_port_created);
  sem_wait(&alive_port_created);

  storage_server_data resp;

  Tree SS_Tree = InitTree();

  i8 numpaths;
  scanf("%hhi", &numpaths);
  InitDirectory(SS_Tree);
  for (i8 i = 0; i < numpaths; i++)
  {
    char filepath[MAX_STR_LEN];
    scanf("%s", filepath);
    // AddAccessibleDir(filepath, SS_Tree);
    RemoveInaccessiblePath(SS_Tree, filepath);
  }

  SendTreeData(SS_Tree, resp.ss_tree);

  resp.port_for_client = port_for_client;
  resp.port_for_nm = port_for_nm;
  resp.port_for_alive = port_for_alive;
  CHECK(getcwd(resp.UUID, MAX_STR_LEN), NULL);

  const i32 sockfd = connect_to_port(NM_SS_PORT);
  send_data_in_packets(&resp, sockfd, sizeof(resp));
  PrintTree(SS_Tree, 0);

  CHECK(close(sockfd), -1);
  return NULL;
}

/**
 * @brief Accept connection requests sent periodically from the naming server to ensure storage server is alive
 *
 * @param arg NULL
 * @return void* NULL
 */
void *alive_relay(void *arg)
{
  (void)arg;

  const i32 serverfd = bind_to_port(0);
  port_for_alive = get_port(serverfd);
  sem_post(&alive_port_created);

  printf("Listening for alive on port %i\n", port_for_alive);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);
    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}

void *nm_communication_init(void *arg)
{
  (void)arg;

  const i32 serverfd = bind_to_port(0);
  port_for_nm = get_port(serverfd);
  sem_post(&nm_port_created);

  printf("Listening for naming server on port %i\n", port_for_nm);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    i32 *clientfd = malloc(sizeof(i32));
    *clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(*clientfd, -1);

    pthread_t naming_server_relay_thread;
    pthread_create(&naming_server_relay_thread, NULL, naming_server_relay, clientfd);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}

enum status send_for_copy(const i32 clientfd)
{
  char path[MAX_STR_LEN];
  enum status code;
  i8 rec_code;
  CHECK(recv(clientfd, &rec_code, sizeof(rec_code), 0), -1);
  while (rec_code == 1)
  {
    CHECK(recv(clientfd, path, MAX_STR_LEN, 0), -1);
    printf("Received %s\n", path);

    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
      if (errno == EACCES)
        code = READ_PERMISSION_DENIED;
      else
        code = NOT_FOUND;
      CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    }
    else
    {
      code = SUCCESS;
      CHECK(send(clientfd, &code, sizeof(code), 0), -1);
      transmit_file_for_writing(file, clientfd);
      fclose(file);
    }

    CHECK(recv(clientfd, &rec_code, sizeof(rec_code), 0), -1);
  }

  return code;
}

enum status receive_from_copy(const i32 clientfd)
{
  enum status code;
  char path[MAX_STR_LEN];
  i8 is_file;
  CHECK(recv(clientfd, &is_file, sizeof(is_file), 0), -1);
  while (is_file == 0 || is_file == 1)
  {
    CHECK(recv(clientfd, path, MAX_STR_LEN, 0), -1);
    printf("Received %s\n", path);
    if (is_file == 0)
    {
      i32 res = mkdir(path, 0777);
      if (res == -1)
      {
        if (errno == EACCES)
          code = CREATE_PERMISSION_DENIED;
        else
          code = INVALID_PATH;
      }
      else
      {
        code = SUCCESS;
      }
      CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    }
    else
    {
      FILE *f = fopen(path, "a");
      if (f == NULL)
      {
        if (errno == EACCES)
          code = WRITE_PERMISSION_DENIED;
        else
          code = INVALID_PATH;
        CHECK(send(clientfd, &code, sizeof(code), 0), -1);
      }
      else
      {
        code = SUCCESS;
        CHECK(send(clientfd, &code, sizeof(code), 0), -1);
        receive_and_write_file(clientfd, f);
      }
    }
    CHECK(recv(clientfd, &is_file, sizeof(is_file), 0), -1);
  }
  return code;
}

/**
 * @brief Handles operations sent via the naming server. Sends back a status code to the naming server.
 *
 * @param arg NULL
 * @return void* NULLg
 */
void *naming_server_relay(void *arg)
{
  const i32 clientfd = *(i32 *)arg;
  free(arg);

  enum operation op;
  CHECK(recv(clientfd, &op, sizeof(op), 0), -1);

  enum status code;
  if (op == CREATE_FILE || op == DELETE_FILE || op == CREATE_FOLDER || op == DELETE_FOLDER)
  {
    char path[MAX_STR_LEN];
    CHECK(recv(clientfd, path, sizeof(path), 0), -1);
    if (op == CREATE_FILE)
    {
      FILE *f = fopen(path, "a");
      if (f == NULL)
      {
        if (errno == EACCES)
          code = WRITE_PERMISSION_DENIED;
        else
          code = NOT_FOUND;
      }
      else
      {
        code = SUCCESS;
        fclose(f);
      }
    }
    else if (op == DELETE_FILE)
    {
      i32 res = remove(path);
      if (res == -1)
      {
        if (errno == EACCES)
          code = DELETE_PERMISSION_DENIED;
        else if (errno == EBUSY)
          code = UNAVAILABLE;
        else
          code = NOT_FOUND;
      }
      else
      {
        code = SUCCESS;
      }
    }
    else if (op == CREATE_FOLDER)
    {
      i32 res = mkdir(path, 0777);
      if (res == -1)
      {
        if (errno == EACCES)
          code = CREATE_PERMISSION_DENIED;
        else if (errno == EEXIST)
          code = ALREADY_EXISTS;
        else
          code = NOT_FOUND;
      }
      else
      {
        code = SUCCESS;
      }
    }
    else if (op == DELETE_FOLDER)
    {
      pid_t pid = fork();
      CHECK(pid, -1);
      if (pid == 0)
      {
        char *args[] = {"rm", "-r", path, NULL};
        execvp("rm", args);
        exit(1); // this line won't be reached if execvp succeeds
      }
      i32 status;
      CHECK(wait(&status), -1);
      if (WIFEXITED(status))
      {
        switch (WEXITSTATUS(status))
        {
        case 0:
          code = SUCCESS;
          break;
        case 1:  // Operation not permitted
        case 13: // Permission denied
        case 30: // Read-only file system
          code = DELETE_PERMISSION_DENIED;
          break;
        case 16: // Resource busy
          code = UNAVAILABLE;
          break;
        default:
          code = NOT_FOUND;
          break;
        }
      }
      else
      {
        code = UNKNOWN_PERMISSION_DENIED;
      }
    }
  }
  else if (op == COPY_FILE || op == COPY_FOLDER)
  {
    enum copy_type ch;

    CHECK(recv(clientfd, &ch, sizeof(ch), 0), -1);

    if (ch == SENDER)
    {
      code = send_for_copy(clientfd);
    }
    else if (ch == RECEIVER)
    {
      code = receive_from_copy(clientfd);
    }
    else
    {
      code = INVALID_OPERATION;
    }
  }
  else
  {
    code = INVALID_OPERATION;
  }

  CHECK(send(clientfd, &code, sizeof(code), 0), -1);

  CHECK(close(clientfd), -1);

  return NULL;
}
