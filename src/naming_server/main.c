#include "../common/headers.h"
#include "headers.h"

void *storage_server_init(void *arg)
{
  // naming server is the server
  (void)arg;
  const i32 serverfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(serverfd, -1);

  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, '\0', sizeof(server_addr));
  memset(&client_addr, '\0', sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(NM_SS_PORT);
  server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

  CHECK(bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), -1);
  CHECK(listen(serverfd, MAX_STORAGE_SERVERS), -1);
  printf("Listening for storage servers on port %i\n", NM_SS_PORT);
  while (1)
  {
    // receive init information from the storage servers
    // also periodically check if each storage server is still alive
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    storage_server_init_response resp;
    CHECK(recv(clientfd, &resp, sizeof(resp), 0), -1)
    printf("%i %i\n", resp.port_for_client, resp.port_for_nm);

    char send_buffer[MAX_STR_LEN] = {0};
    strcpy(send_buffer, "sent from naming server");
    CHECK(send(clientfd, send_buffer, MAX_STR_LEN, 0), -1);

    CHECK(close(clientfd), -1);
  }

  CHECK(close(serverfd), -1);
  return NULL;
}

void *client_relay(void *arg)
{
  // naming server is the server
  (void)arg;

  return NULL;
}

void *storage_server_relay(void *arg)
{
  // naming server is the client
  (void)arg;
  return NULL;
}

int main()
{
  pthread_t storage_server_init_thread, client_relay_thread, storage_server_relay_thread;

  pthread_create(&storage_server_init_thread, NULL, storage_server_init, NULL);
  pthread_create(&client_relay_thread, NULL, client_relay, NULL);
  pthread_create(&storage_server_relay_thread, NULL, storage_server_relay, NULL);

  pthread_join(storage_server_init_thread, NULL);
  pthread_join(client_relay_thread, NULL);
  pthread_join(storage_server_relay_thread, NULL);
  return 0;
}