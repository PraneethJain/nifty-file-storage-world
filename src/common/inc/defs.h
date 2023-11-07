#ifndef __DEFS_H
#define __DEFS_H

#include <stdint.h>

#define LOCALHOST "127.0.0.1"
#define NM_SS_PORT 18000
#define NM_CLIENT_PORT 18001
#define MAX_STR_LEN 1024
#define MAX_DIRNAME_LEN 32
#define MAX_CONNECTIONS 16

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define ERROR_PRINT(fmt, args...)                                                                                      \
  fprintf(stderr, C_RED "[ERROR] %s:%d:%s()" fmt C_RESET, __FILE__, __LINE__, __func__, ##args)

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...)                                                                                      \
  fprintf(stderr, C_YELLOW "[DEBUG] %s:%d:%s(): " fmt C_RESET, __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#endif