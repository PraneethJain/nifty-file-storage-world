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

  resp.ss_tree = InitTree();
  // TODO: take user input of all accessible paths and fill in directory structure in resp

  i8 numpaths;
  scanf("%hhd", &numpaths);
  for (i8 i = 0; i < numpaths; i++)
  {
    char filepath[MAX_STR_LEN];
    scanf("%s", filepath);
    AddAccessibleDir(filepath, resp.ss_tree);
  }

  PrintTree(resp.ss_tree, 0);

  resp.port_for_client = port_for_client;
  resp.port_for_nm = port_for_nm;
  resp.port_for_alive = port_for_alive;

  const i32 sockfd = connect_to_port(NM_SS_PORT);
  CHECK(send(sockfd, &resp, sizeof(resp), 0), -1);

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

/**
 * @brief Handles operations sent via the naming server. Sends back a status code to the naming server.
 *
 * @param arg NULL
 * @return void* NULLg
 */
void *naming_server_relay(void *arg)
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
    // clientfd here is the file descriptor of the naming server
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    enum operation op;
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1);

    i32 status = -1;
    if (op == COPY_FILE || op == COPY_FOLDER)
    {
      // receive 2 paths
      status = 2;
    }
    else
    {
      char path[MAX_STR_LEN];
      CHECK(recv(clientfd, path, sizeof(path), 0), -1);
      status = 1;
    }

    CHECK(send(clientfd, &status, sizeof(status), 0), -1);

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}
