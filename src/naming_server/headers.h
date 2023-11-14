#ifndef __NAMING_SERVER_HEADERS_H
#define __NAMING_SERVER_HEADERS_H

#include "../common/headers.h"

#define LOG(fmt, args...)                                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    FILE *ptr = fopen("logfile.log", "a");                                                                             \
    time_t currentTime;                                                                                                \
    time(&currentTime);                                                                                                \
    char timestamp[20];                                                                                                \
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&currentTime));                              \
    fprintf(ptr, "[%s] - " fmt, timestamp, ##args);                                                                    \
    fclose(ptr);                                                                                                       \
  } while (0)

// nm_to_ss.c
void add_connected_storage_server(storage_server_data data);
void *storage_server_init(void *arg);
void *alive_checker(void *arg);
i32 ss_client_port_from_path(const char *path);
i32 ss_nm_port_from_path(const char *path);
i32 ss_nm_port_new();

// nm_to_client.c
void *client_relay(void *arg);
void *client_init(void *arg);

#endif
