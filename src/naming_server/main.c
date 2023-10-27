#include "../common/headers.h"
#include "headers.h"

struct connected_storage_servers
{
  i32 size;
  storage_server_data storage_servers[MAX_STORAGE_SERVERS];
} connected_storage_servers;

void remove_connected_storage_server(i32 index)
{
  if (index < 0 || index >= connected_storage_servers.size)
  {
    ERROR_PRINT("Invalid index\n");
  }

  for (i32 i = index; i + 1 < connected_storage_servers.size; ++i)
  {
    connected_storage_servers.storage_servers[i] = connected_storage_servers.storage_servers[i + 1];
  }
  connected_storage_servers.size--;

  // remove this storage server's stuff from directory structure
}

void add_connected_storage_server(storage_server_data data)
{
  connected_storage_servers.storage_servers[connected_storage_servers.size++] = data;
  // add this storage server's stuff to directory structure
}

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
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    storage_server_data resp;
    CHECK(recv(clientfd, &resp, sizeof(resp), 0), -1)
    printf("%i %i\n", resp.port_for_client, resp.port_for_nm);

    CHECK(close(clientfd), -1);
    add_connected_storage_server(resp);
  }

  CHECK(close(serverfd), -1);
  return NULL;
}

void *storage_server_checker(void *arg)
{
  // naming server is the client
  // periodically check if each storage server is alive
  (void)arg;
  while (1)
  {
    sleep(3);
    for (i32 i = 0; i < connected_storage_servers.size; ++i)
    {
      const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
      CHECK(sockfd, -1);

      struct sockaddr_in addr;
      memset(&addr, '\0', sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(connected_storage_servers.storage_servers[i].port_for_nm);
      addr.sin_addr.s_addr = inet_addr(LOCALHOST);
      if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
      {
        if (errno == 111) // Connection refused
        {
          printf("Storage server %i has disconnected!\n", i);
          remove_connected_storage_server(i);
          break;
        }
        else
        {
          ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));
          exit(1);
        }
      }
      else
      {
        printf("Storage server %i is alive!\n", i);
      }

      CHECK(close(sockfd), -1);
    }
  }
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
  connected_storage_servers.size = 0;
  pthread_t storage_server_init_thread, client_relay_thread, storage_server_relay_thread, storage_server_checker_thread;

  pthread_create(&storage_server_init_thread, NULL, storage_server_init, NULL);
  pthread_create(&client_relay_thread, NULL, client_relay, NULL);
  pthread_create(&storage_server_relay_thread, NULL, storage_server_relay, NULL);
  pthread_create(&storage_server_checker_thread, NULL, storage_server_checker, NULL);

  pthread_join(storage_server_init_thread, NULL);
  pthread_join(client_relay_thread, NULL);
  pthread_join(storage_server_relay_thread, NULL);
  pthread_join(storage_server_checker_thread, NULL);
  return 0;
}