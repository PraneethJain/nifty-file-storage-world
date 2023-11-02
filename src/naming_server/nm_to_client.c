#include "../common/headers.h"
#include "headers.h"

void send_client_port(const i32 clientfd)
{
  char path[MAX_STR_LEN];
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);

  const i32 port = ss_client_port_from_path(path);
  CHECK(send(clientfd, &port, sizeof(port), 0), -1);
}

void send_nm_op(const i32 clientfd, const enum operation op)
{
  char path[MAX_STR_LEN];
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);

  // naming server is the client
  const i32 port = ss_nm_port_from_path(path);
  const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(LOCALHOST);
  CHECK(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)), -1);

  CHECK(send(sockfd, &op, sizeof(op), 0), -1);
  CHECK(send(sockfd, path, sizeof(path), 0), -1);

  // send status code received from ss to client
  i32 status;
  CHECK(recv(sockfd, &status, sizeof(status), 0), -1);
  CHECK(send(clientfd, &status, sizeof(status), 0), -1);

  close(sockfd);
}

void *client_init(void *arg)
{
  // naming server is the server
  (void)arg;
  const i32 serverfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(serverfd, -1);

  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, '\0', sizeof(server_addr));
  memset(&client_addr, '\0', sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(NM_CLIENT_PORT);
  server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

  CHECK(bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), -1);
  CHECK(listen(serverfd, MAX_CLIENTS), -1);
  printf("Listening for clients on port %i\n", NM_CLIENT_PORT);
  while (1)
  {
    // receive init information from the clients
    socklen_t addr_size = sizeof(client_addr);
    i32 *clientfd = malloc(sizeof(i32));
    *clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(*clientfd, -1);

    pthread_t client_relay_thread;
    pthread_create(&client_relay_thread, NULL, client_relay, clientfd);
  }
  // maybe join client_relays

  CHECK(close(serverfd), -1);

  return NULL;
}

void *client_relay(void *arg)
{
  const i32 clientfd = *(i32 *)arg;
  while (1)
  {
    enum operation op;
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1)
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
      send_nm_op(clientfd, op);
      break;
    case COPY_FILE:
      break;
    case COPY_FOLDER:
      break;
    }
  }

  return NULL;
}
