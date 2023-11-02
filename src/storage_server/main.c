#include "../common/headers.h"
#include "headers.h"

i32 port_for_client = -1;
i32 port_for_nm = -1;
i32 port_for_alive = -1;
sem_t client_port_created;
sem_t nm_port_created;
sem_t alive_port_created;

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

void *client_relay(void *arg)
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
  port_for_client = server_addr.sin_port;
  sem_post(&client_port_created);

  printf("Listening for clients on port %i\n", port_for_client);
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    char recv_buffer[MAX_STR_LEN] = {0};
    CHECK(recv(clientfd, recv_buffer, MAX_STR_LEN, 0), -1)
    printf("%s\n", recv_buffer);

    char send_buffer[MAX_STR_LEN] = {0};
    strcpy(send_buffer, "sent from TCP server");
    CHECK(send(clientfd, send_buffer, MAX_STR_LEN, 0), -1);

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

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
      printf("received path %s\n", path);
      status = 1;
    }

    CHECK(send(clientfd, &status, sizeof(status), 0), -1);

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}

int main()
{
  sem_init(&client_port_created, 0, 0);
  sem_init(&nm_port_created, 0, 0);
  sem_init(&alive_port_created, 0, 0);

  pthread_t init_storage_server_thread, client_relay_thread, alive_thread, naming_server_relay_thread;
  pthread_create(&init_storage_server_thread, NULL, init_storage_server, NULL);
  pthread_create(&client_relay_thread, NULL, client_relay, NULL);
  pthread_create(&alive_thread, NULL, alive_relay, NULL);
  pthread_create(&naming_server_relay_thread, NULL, naming_server_relay, NULL);

  pthread_join(init_storage_server_thread, NULL);
  pthread_join(client_relay_thread, NULL);
  pthread_join(alive_thread, NULL);
  pthread_join(naming_server_relay_thread, NULL);

  return 0;
}
