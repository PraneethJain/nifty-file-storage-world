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
    fprintf(ptr, "[%s] %s:%d:%s() - " fmt, timestamp, __FILE__, __LINE__, __func__, ##args);                           \
    fclose(ptr);                                                                                                       \
  } while (0)

#define LOG_RECV(sockfd, data)                                                                                         \
  LOG("Receiving " #data " from " #sockfd "\n");                                                                       \
  RECV(sockfd, data);                                                                                                  \
  LOG("Received " #data " from " #sockfd "\n");

#define LOG_SEND(sockfd, data)                                                                                         \
  LOG("Sending " #data " to " #sockfd "\n");                                                                           \
  SEND(sockfd, data);                                                                                                  \
  LOG("Sent " #data " to " #sockfd "\n");

// nm_to_ss.c
void add_connected_storage_server(storage_server_data data);
void *storage_server_init(void *arg);
void *alive_checker(void *arg);
i32 ss_client_port_from_path(const char *path);
i32 ss_nm_port_from_path(const char *path);
i32 ss_nm_port_new();
storage_server_data *ss_from_path(const char *path);
storage_server_data *MinSizeStorageServer();

// nm_to_client.c
void *client_relay(void *arg);
void *client_init(void *arg);

extern Tree NM_Tree;
extern pthread_mutex_t tree_lock;

#endif
