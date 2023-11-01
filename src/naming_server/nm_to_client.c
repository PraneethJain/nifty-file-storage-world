#include "../common/headers.h"
#include "headers.h"

void path_to_ss(const i32 clientfd)
{
  char path[MAX_STR_LEN];
  printf("receiving\n");
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);
  printf("received %s\n", path);

  const i32 port = ss_client_port_from_path(path);
  CHECK(send(clientfd, &port, sizeof(port), 0), -1);
}

void *client_init(void *arg)
{
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
    printf("clientfd is %i\n", *clientfd);
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
  // naming server is the server
  const i32 clientfd = *(i32 *)arg;
  printf("clientfd is %i\n", clientfd);
  while (1)
  {
    enum operation op;
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1)
    switch (op)
    {
    case READ:
    case WRITE:
    case METADATA:
      path_to_ss(clientfd);
      break;
    case CREATE_FILE:
      break;
    case DELETE_FILE:
      break;
    case CREATE_FOLDER:
      break;
    case DELETE_FOLDER:
      break;
    case COPY_FILE:
      break;
    case COPY_FOLDER:
      break;
    }
  }

  return NULL;
}
