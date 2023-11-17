#ifndef _COMMON_HEADERS_H
#define _COMMON_HEADERS_H

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "inc/colors.h"
#include "inc/defs.h"
#include "inc/tree.h"

// #define CACHE_SIZE 10

// typedef struct cache
// {
//   char *path;
//   int SSID;
//   struct cache *next;
// } cache;

typedef struct storage_server_data
{
  i32 port_for_client;
  i32 port_for_nm;
  i32 port_for_alive;
  char ss_tree[MAX_STR_LEN * 50];
} storage_server_data;

typedef struct metadata
{
  time_t last_modified_time;
  time_t last_access_time;
  time_t last_status_change_time;
  u64 size;
  mode_t mode;
} metadata;

enum operation
{
  READ,
  WRITE,
  METADATA,
  CREATE_FILE,
  DELETE_FILE,
  CREATE_FOLDER,
  DELETE_FOLDER,
  COPY_FILE,
  COPY_FOLDER,
  DISCONNECT
};

enum status
{
  SUCCESS,
  INVALID_PATH,
  INVALID_OPERATION,
  NOT_FOUND,
  UNAVAILABLE,
  READ_PERMISSION_DENIED,
  WRITE_PERMISSION_DENIED,
  CREATE_PERMISSION_DENIED,
  DELETE_PERMISSION_DENIED,
  UNKNOWN_PERMISSION_DENIED,
  NON_EMPTY_DIRECTORY
};

enum copy_type
{
  SENDER,
  RECEIVER
};

// network.c
i32 connect_to_port(const i32 port);
i32 bind_to_port(const i32 port);
i32 get_port(const i32 fd);
void send_file(FILE *f, const i32 sockfd);
void receive_and_print_file(const i32 sockfd);

void transmit_file_for_writing(FILE *f, const i32 sockfd);
void receive_and_transmit_file(const i32 from_sockfd, const i32 to_sockfd);
void receive_and_write_file(const i32 from_sockfd, FILE *f);

#define CHECK(actual_value, error_value)                                                                               \
  if ((actual_value) == error_value)                                                                                   \
  {                                                                                                                    \
    ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));                                                \
    exit(1);                                                                                                           \
  }

#endif
