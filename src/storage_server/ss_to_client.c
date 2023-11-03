#include "../common/headers.h"
#include "headers.h"

void *client_relay(void *arg)
{
  // storage server is the server
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

    char path[MAX_STR_LEN];
    CHECK(recv(clientfd, path, MAX_STR_LEN, 0), -1)
    printf("%s\n", path);

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);

  return NULL;
}
