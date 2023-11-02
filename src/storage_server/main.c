#include "../common/headers.h"
#include "headers.h"

i32 port_for_client = -1;
i32 port_for_nm = -1;
i32 port_for_alive = -1;
sem_t client_port_created;
sem_t nm_port_created;
sem_t alive_port_created;

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
