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
    CHECK(recv(clientfd, path, MAX_STR_LEN, 0), -1)
    printf("Recieved path %s\n", path);

    FILE *file = fopen(path, "r");
    CHECK(file, NULL);
    send_file(file, clientfd);
    fclose(file);

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}
