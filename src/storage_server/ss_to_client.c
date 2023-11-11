/**
 * @file ss_to_client.c
 * @brief Communication between a storage server and a client
 * @details
 * - Handles operations sent directly to a storage server from a client
 */

#include "../common/headers.h"
#include "headers.h"

/**
 * @brief Handles operations from client directly sent to storage server
 *
 * @param arg NULL
 * @return void* NULL
 */
void *client_relay(void *arg)
{
  // TODO
  (void)arg;

  const i32 serverfd = bind_to_port(0);
  port_for_client = get_port(serverfd);
  sem_post(&client_port_created);

  printf("Listening for clients on port %i\n", port_for_client);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    enum operation op;
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1);
    char path[MAX_STR_LEN];
    CHECK(recv(clientfd, path, sizeof(path), 0), -1)
    printf("Recieved path %s\n", path);

    enum status code;
    if (op == READ)
    {
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
        send_file(file, clientfd);
        fclose(file);
      }
    }
    else if (op == WRITE)
    {
      FILE *file = fopen(path, "w");
      if (file == NULL)
      {
        if (errno == EACCES)
          code = WRITE_PERMISSION_DENIED;
        else
          code = NOT_FOUND;
        CHECK(send(clientfd, &code, sizeof(code), 0), -1);
      }
      else
      {
        code = SUCCESS;
        CHECK(send(clientfd, &code, sizeof(code), 0), -1);
        char buffer[MAX_STR_LEN];
        CHECK(recv(clientfd, buffer, sizeof(buffer), 0), -1);
        fwrite(buffer, strlen(buffer), 1, file);
        fclose(file);
      }
    }
    else if (op == METADATA)
    {
      struct stat fileinfo;
      i32 res = stat(path, &fileinfo);
      if (res == -1)
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

        metadata meta;
        meta.last_modified_time = fileinfo.st_mtime;
        meta.last_access_time = fileinfo.st_atime;
        meta.last_status_change_time = fileinfo.st_ctime;
        meta.size = fileinfo.st_size;
        meta.mode = fileinfo.st_mode;

        CHECK(send(clientfd, &meta, sizeof(meta), 0), -1);
      }
    }
    else
    {
    }

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}
