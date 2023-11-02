#include "../common/headers.h"
#include "headers.h"

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
