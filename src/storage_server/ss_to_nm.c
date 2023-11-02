#include "../common/headers.h"
#include "headers.h"

void *init_storage_server(void *arg)
{
  // storage server is the client
  (void)arg;
  const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(sockfd, -1);

  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(NM_SS_PORT);
  addr.sin_addr.s_addr = inet_addr(LOCALHOST);
  CHECK(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)), -1);

  sem_wait(&client_port_created);
  sem_wait(&nm_port_created);
  sem_wait(&alive_port_created);

  storage_server_data resp;
  resp.port_for_client = port_for_client;
  resp.port_for_nm = port_for_nm;
  resp.port_for_alive = port_for_alive;
  // also fill in the directory structure in resp
  CHECK(send(sockfd, &resp, sizeof(resp), 0), -1);

  CHECK(close(sockfd), -1);
  return NULL;
}

void *alive_relay(void *arg)
{
  // storage server is the server
  (void)arg;

  const i32 serverfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(serverfd, -1);

  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, '\0', sizeof(server_addr));
  memset(&client_addr, '\0', sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = 0;
  server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

  CHECK(bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), -1);
  CHECK(listen(serverfd, MAX_CLIENTS), -1);

  socklen_t len = sizeof(server_addr);
  CHECK(getsockname(serverfd, (struct sockaddr *)&server_addr, &len), -1);
  port_for_alive = ntohs(server_addr.sin_port);
  sem_post(&alive_port_created);

  printf("Listening for alive on port %i\n", port_for_alive);
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

void *naming_server_relay(void *arg)
{
  // storage server is the server
  (void)arg;

  const i32 serverfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(serverfd, -1);

  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, '\0', sizeof(server_addr));
  memset(&client_addr, '\0', sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = 0;
  server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

  CHECK(bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), -1);
  CHECK(listen(serverfd, MAX_CLIENTS), -1);

  socklen_t len = sizeof(server_addr);
  CHECK(getsockname(serverfd, (struct sockaddr *)&server_addr, &len), -1);
  port_for_nm = ntohs(server_addr.sin_port);
  sem_post(&nm_port_created);

  printf("Listening for naming server on port %i\n", port_for_nm);
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    enum operation op;
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1);

    i32 status = -1;
    if (op == COPY_FILE || op == COPY_FOLDER)
    {
      // receive 2 paths
    }
    else
    {
      // receive path
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
