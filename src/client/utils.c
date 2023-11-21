/**
 * @file utils.c
 * @brief Common utilities for the client
 * @details
 * - Basic Input Output
 * - Error handling
 */

#include "../common/headers.h"
#include "headers.h"

/**
 * @brief Get the operation number after prompting the user
 *
 * @return i8
 */
i8 get_operation()
{
  printf(C_YELLOW "\nOperations:-\n" C_CYAN "1.Read\n"
                  "2.Write\n"
                  "3.Metadata\n" C_GREEN "4.Create file\n" C_RED "5.Delete file\n" C_GREEN "6.Create folder\n" C_RED
                  "7.Delete folder\n" C_WHITE "8.Copy file\n" C_WHITE "9.Copy folder\n" C_BLUE "10.Print Tree\n" C_BLACK
                  "11.Exit\n");
  i8 op_int = -1;
  while (op_int < 1 || op_int >= END_OPERATION)
  {
    printf(C_YELLOW "Choice: " C_RESET);
    char buf[MAX_STR_LEN];
    fgets(buf, MAX_STR_LEN, stdin);
    char *endptr;
    op_int = strtol(buf, &endptr, 10);
    if ((errno != 0 && op_int == 0) || endptr == buf)
    {
      continue;
    }
  }
  return op_int - 1;
}

/**
 * @brief Checks for errors in the path, returning true if an error is present
 *
 * @param path
 * @return true
 * @return false
 */
bool path_error(const char *path)
{
  const i16 len = strlen(path);
  if (len == 0 || (len == 1 && isspace(path[0])))
    return true;
  for (i16 i = 0; i < len; i++)
  {
    if ((i != len - 1 && isspace(path[i])) || (i != 0 && path[i - 1] == '/' && path[i] == '/'))
      return true;
  }
  return false;
}

/**
 * @brief Read a path after prompting, with error handling
 *
 * @param path_buffer
 */
void read_path(char *path_buffer)
{
  printf(C_YELLOW "Enter path: " C_RESET);
  fgets(path_buffer, MAX_STR_LEN, stdin);
  path_buffer[strcspn(path_buffer, "\n")] = 0;

  while (path_error(path_buffer))
  {
    printf(C_YELLOW "Enter path: " C_RESET);
    fgets(path_buffer, MAX_STR_LEN, stdin);
    path_buffer[strcspn(path_buffer, "\n")] = 0;
  }
}

/**
 * @brief Print the corresponding error message to the status code
 *
 * @param code
 */
void print_error(enum status code)
{
  printf("\n" C_RED);
  switch (code)
  {
  case SUCCESS:
    break;
  case INVALID_PATH:
    printf("The path is invalid!");
    break;
  case INVALID_OPERATION:
    printf("Bad operation!");
    break;
  case NOT_FOUND:
    printf("The path is not found!");
    break;
  case UNAVAILABLE:
    printf("The resource is busy!");
    break;
  case READ_PERMISSION_DENIED:
    printf("Read permission denied!");
    break;
  case WRITE_PERMISSION_DENIED:
    printf("Write permission denied!");
    break;
  case CREATE_PERMISSION_DENIED:
    printf("Create permission denied!");
    break;
  case DELETE_PERMISSION_DENIED:
    printf("Delete permission denied!");
    break;
  case UNKNOWN_PERMISSION_DENIED:
    printf("Permission denied!");
    break;
  case INVALID_TYPE:
    printf("File/Folder type does not match!");
    break;
  case RECURSIVE_COPY:
    printf("Cannot copy a file/folder inside its subdirectory!");
    break;
  case ALREADY_EXISTS:
    printf("The path already exists!");
    break;
  }
  printf("\n" C_RESET);
}

/**
 * @brief Print file permissions
 *
 * @param mode
 */
void print_mode(mode_t mode)
{
  switch (mode & S_IFMT)
  {
  case S_IFBLK:
    putchar('b');
    break;
  case S_IFCHR:
    putchar('c');
    break;
  case S_IFDIR:
    putchar('d');
    break;
  case S_IFIFO:
    putchar('p');
    break;
  case S_IFLNK:
    putchar('l');
    break;
  case S_IFREG:
    putchar('-');
    break;
  case S_IFSOCK:
    putchar('s');
    break;
  default:
    putchar('?');
    break;
  }

  putchar((mode & S_IRUSR) ? 'r' : '-');
  putchar((mode & S_IWUSR) ? 'w' : '-');
  if (mode & S_ISUID)
    putchar((mode & S_IXUSR) ? 's' : 'S');
  else
    putchar((mode & S_IXUSR) ? 'x' : '-');

  putchar((mode & S_IRGRP) ? 'r' : '-');
  putchar((mode & S_IWGRP) ? 'w' : '-');
  if (mode & S_ISGID)
    putchar((mode & S_IXGRP) ? 's' : 'S');
  else
    putchar((mode & S_IXGRP) ? 'x' : '-');

  putchar((mode & S_IROTH) ? 'r' : '-');
  putchar((mode & S_IWOTH) ? 'w' : '-');
  if (mode & S_ISVTX)
    putchar((mode & S_IXOTH) ? 't' : 'T');
  else
    putchar((mode & S_IXOTH) ? 'x' : '-');

  putchar(' ');
}

/**
 * @brief Prepend the path with the corresponding redundancy number
 *
 * @param i redundancy number
 * @param path path to prepend to
 * @param buf output path
 */
void fill_rd_path(const i32 i, const char *path, char *buf)
{
  sprintf(buf, ".rd%i/", i);
  strcat(buf, path);
}

/**
 * @brief Delete redundancies given a path
 *
 * @param nm_sockfd
 * @param op
 * @param path
 */
void delete_rd_paths(const i32 nm_sockfd, enum operation op, const char *path)
{
  for (int i = 1; i <= 3; ++i)
  {
    char rd_path[MAX_STR_LEN];
    fill_rd_path(i, path, rd_path);
    SEND(nm_sockfd, op);
    SEND(nm_sockfd, rd_path);
    enum status code;
    RECV(nm_sockfd, code);
  }
}

/**
 * @brief Print the metadata
 *
 * @param meta
 */
void print_metadata(metadata meta)
{
  printf("Size: %lu\n", meta.size);
  printf("Last accessed: %s\n", ctime(&meta.last_access_time));
  printf("Last modified: %s\n", ctime(&meta.last_modified_time));
  printf("Last status change: %s\n", ctime(&meta.last_status_change_time));
  print_mode(meta.mode);
  printf("\n");
}