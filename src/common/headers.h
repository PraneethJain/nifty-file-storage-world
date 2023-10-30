#ifndef _COMMON_HEADERS_H
#define _COMMON_HEADERS_H

#include <arpa/inet.h>
#include <ctype.h>
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
#include <sys/types.h>
#include <unistd.h>

#include "inc/colors.h"
#include "inc/defs.h"
#include "inc/tree.h"

#define LOCALHOST "127.0.0.1"
#define NM_SS_PORT 18000
#define NM_CLIENT_PORT 18001
#define MAX_STR_LEN 1024
#define MAX_CLIENTS 16
#define MAX_STORAGE_SERVERS 16

typedef struct storage_server_data
{
  i32 port_for_client;
  i32 port_for_nm;
  // directory structure
} storage_server_data;

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
  COPY_FOLDER
};

#define CHECK(actual_value, error_value)                                                                               \
  if ((actual_value) == error_value)                                                                                   \
  {                                                                                                                    \
    ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));                                                \
    exit(1);                                                                                                           \
  }

#endif