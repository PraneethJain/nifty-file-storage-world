#ifndef __STORAGE_SERVER_HEADERS_H
#define __STORAGE_SERVER_HEADERS_H

#include "../common/headers.h"

extern i32 port_for_client;
extern i32 port_for_nm;
extern i32 port_for_alive;

extern sem_t client_port_created;
extern sem_t nm_port_created;
extern sem_t alive_port_created;

// ss_to_client.c
void *client_relay(void *arg);

// ss_to_nm.c
void *init_storage_server(void *arg);
void *alive_relay(void *arg);
void *naming_server_relay(void *arg);


#endif
