/**
 * @file main.c
 * @brief Entry point for the naming server
 * @details
 * Initialize threads for:
 * - Receiving initial information from storage servers
 * - Periodically checking if each of those storage servers is alive
 * - Receiving connections from clients
 */

#include "../common/headers.h"
#include "headers.h"

Tree NM_Tree;

int main()
{
  NM_Tree = InitTree();
  pthread_t storage_server_init_thread, alive_checker_thread;
  pthread_t client_relay_thread;

  pthread_create(&storage_server_init_thread, NULL, storage_server_init, NULL);
  pthread_create(&alive_checker_thread, NULL, alive_checker, NULL);
  pthread_create(&client_relay_thread, NULL, client_init, NULL);

  pthread_join(storage_server_init_thread, NULL);
  pthread_join(alive_checker_thread, NULL);
  pthread_join(client_relay_thread, NULL);

  return 0;
}
