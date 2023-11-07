/**
 * @file nm_to_ss.c
 * @brief Communication between the naming server and storage servers
 * @details
 * - Stores all the currently connected storage servers
 * - Manages adding and removing these servers whenever needed
 * - Retrieves storage server data from paths provided
 */

#include "../common/headers.h"
#include "headers.h"

extern Tree NM_Tree;

struct
{
  i32 size;
  storage_server_data storage_servers[MAX_CONNECTIONS];
} connected_storage_servers = {0};

/**
 * @brief Remove a disconnected storage server
 *
 * @param index
 */
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

  // TODO: remove this storage server's stuff from directory structure
}

/**
 * @brief Add a connected storage server
 *
 * @param data
 */
void add_connected_storage_server(storage_server_data data)
{
  connected_storage_servers.storage_servers[connected_storage_servers.size].port_for_alive = data.port_for_alive;
  connected_storage_servers.storage_servers[connected_storage_servers.size].port_for_client = data.port_for_client;
  connected_storage_servers.storage_servers[connected_storage_servers.size].port_for_nm = data.port_for_nm;

  Tree temp = ReceiveTreeData(data.ss_tree);
  MergeTree(NM_Tree, temp, data.port_for_nm);

  PrintTree(NM_Tree, 0);

  connected_storage_servers.size++;
  // TODO: add this storage server's stuff to directory structure
}

/**
 * @brief Receive initial port and directory information from all new storage servers
 *
 * @param arg NULL
 * @return void* NULL
 */
void *storage_server_init(void *arg)
{
  (void)arg;

  const i32 serverfd = bind_to_port(NM_SS_PORT);
  printf("Listening for storage servers on port %i\n", NM_SS_PORT);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    const i32 clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(clientfd, -1);

    storage_server_data resp;
    CHECK(recv(clientfd, &resp, sizeof(resp), 0), -1)
    printf("%i %i %i\n", resp.port_for_client, resp.port_for_nm, resp.port_for_alive);

    CHECK(close(clientfd), -1);
    add_connected_storage_server(resp);
  }

  CHECK(close(serverfd), -1);
  return NULL;
}

/**
 * @brief Periodically check if each storage server is still alive.
 * Disconnect the ones that have crashed.
 *
 * @param arg NULL
 * @return void* NULL
 */
void *alive_checker(void *arg)
{
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
      addr.sin_port = htons(connected_storage_servers.storage_servers[i].port_for_alive);
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

/**
 * @brief Finds the storage server corresponding to the path and returns its data
 *
 * @param path
 * @return storage_server_data
 */
storage_server_data ss_from_path(char *path)
{
  (void)path;
  // TODO
  return connected_storage_servers.storage_servers[0];
}

/**
 * @brief Finds the storage server client port corresponding to the path
 *
 * @param path
 * @return i32 storage server client port
 */
i32 ss_client_port_from_path(char *path)
{
  return ss_from_path(path).port_for_client;
}

/**
 * @brief Finds the storage server nm port corresponding to the path
 *
 * @param path
 * @return i32 storage server nm port
 */
i32 ss_nm_port_from_path(char *path)
{
  return ss_from_path(path).port_for_nm;
}
