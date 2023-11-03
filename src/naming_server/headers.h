#ifndef __NAMING_SERVER_HEADERS_H
#define __NAMING_SERVER_HEADERS_H

#include "../common/headers.h"

// nm_to_ss.c
void remove_connected_storage_server(i32 index);
void add_connected_storage_server(storage_server_data data);
void *storage_server_init(void *arg);
void *alive_checker(void *arg);
i32 ss_client_port_from_path(char *path);
i32 ss_nm_port_from_path(char *path);

// nm_to_client.c
void *client_relay(void *arg);
void *client_init(void *arg);

#endif
