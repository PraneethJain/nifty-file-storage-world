#ifndef _COMMON_HEADERS_H
#define _COMMON_HEADERS_H

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "inc/colors.h"
#include "inc/defs.h"
#include "inc/tree.h"

typedef struct storage_server_data
{
  i32 port_for_client;
  i32 port_for_nm;
  i32 port_for_alive;
  char UUID[MAX_STR_LEN];
  char ss_tree[MAX_STR_LEN * 2000];
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
  PRINT_TREE,
  ACK,
  DISCONNECT,
  END_OPERATION
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
void send_data_in_packets(void *buffer, const i32 sockfd, u32 buffer_length);
void receive_data_in_packets(void *buffer, const i32 sockfd, u32 buffer_length);

#define CHECK(actual_value, error_value)                                                                               \
  if ((actual_value) == error_value)                                                                                   \
  {                                                                                                                    \
    ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));                                                \
    exit(1);                                                                                                           \
  }

#define RECV(sockfd, data) CHECK(recv(sockfd, &data, sizeof(data), 0), -1)
#define SEND(sockfd, data) CHECK(send(sockfd, &data, sizeof(data), 0), -1)

#endif
