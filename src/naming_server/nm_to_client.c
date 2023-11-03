#include "../common/headers.h"
#include "headers.h"

void send_client_port(const i32 clientfd)
{
  char path[MAX_STR_LEN];
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);

  const i32 port = ss_client_port_from_path(path);
  CHECK(send(clientfd, &port, sizeof(port), 0), -1);
}

void send_nm_op_single(const i32 clientfd, const enum operation op)
{
  char path[MAX_STR_LEN];
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);

  // naming server is the client
  const i32 port = ss_nm_port_from_path(path);
  const i32 sockfd = connect_to_port(port);

  CHECK(send(sockfd, &op, sizeof(op), 0), -1);
  CHECK(send(sockfd, path, sizeof(path), 0), -1);

  // send status code received from ss to client
  i32 status;
  CHECK(recv(sockfd, &status, sizeof(status), 0), -1);
  CHECK(send(clientfd, &status, sizeof(status), 0), -1);

  close(sockfd);
}

void send_nm_op_double(const i32 clientfd, const enum operation op)
{
  // TODO

  (void)op;
  char from_path[MAX_STR_LEN];
  char to_path[MAX_STR_LEN];
  CHECK(recv(clientfd, from_path, sizeof(to_path), 0), -1);
  CHECK(recv(clientfd, to_path, sizeof(to_path), 0), -1);

  // naming server is the client
  const i32 from_port = ss_nm_port_from_path(from_path);
  const i32 to_port = ss_nm_port_from_path(to_path);

  const i32 from_sockfd = connect_to_port(from_port);
  const i32 to_sockfd = connect_to_port(to_port);

  // CHECK(send(sockfd, &op, sizeof(op), 0), -1);
  // CHECK(send(sockfd, path, sizeof(path), 0), -1);

  // send status code received from ss to client

  close(from_sockfd);
  close(to_sockfd);
}

void *client_init(void *arg)
{
  // naming server is the server
  (void)arg;

  const i32 serverfd = bind_to_port(NM_CLIENT_PORT);
  printf("Listening for clients on port %i\n", NM_CLIENT_PORT);
  struct sockaddr_in client_addr;
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
  free(arg);
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
      send_nm_op_single(clientfd, op);
      break;
    case COPY_FILE:
    case COPY_FOLDER:
      send_nm_op_double(clientfd, op);
      break;
    }
  }

  return NULL;
}
