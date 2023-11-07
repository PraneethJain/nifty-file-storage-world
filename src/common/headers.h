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
#include <unistd.h>

#include "inc/colors.h"
#include "inc/defs.h"
#include "inc/tree.h"



typedef struct storage_server_data
{
  i32 port_for_client;
  i32 port_for_nm;
  i32 port_for_alive;
  Tree ss_tree;
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

// network.c
i32 connect_to_port(const i32 port);
i32 bind_to_port(const i32 port);
i32 get_port(const i32 fd);

#define CHECK(actual_value, error_value)                                                                               \
  if ((actual_value) == error_value)                                                                                   \
  {                                                                                                                    \
    ERROR_PRINT("failed with errno %i (%s)\n", errno, strerror(errno));                                                \
    exit(1);                                                                                                           \
  }

#endif
