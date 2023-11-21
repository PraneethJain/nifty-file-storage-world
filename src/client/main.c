/**
 * @file main.c
 * @brief Entry point for a client
 * @details
 * - Command line interface for a client
 * - Reads operations and sends to the naming server and storage servers
 */

#include "../common/headers.h"
#include "headers.h"

i8 get_operation()
{
  printf("\nOperations:-\n1.Read\n2.Write\n3.Metadata\n4.Create file\n5.Delete file\n6.Create folder\n7.Delete "
         "folder\n8.Copy file\n9.Copy folder\n10.Exit\n");
  i8 op_int = -1;
  while (op_int < 1 || op_int > 10)
  {
    printf("Enter the number of the operation that you wish to perform: ");
    scanf("%hhi", &op_int);
  }
  return op_int - 1;
}

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

void read_path(char *path_buffer)
{
  fgets(path_buffer, MAX_STR_LEN, stdin);
  path_buffer[strcspn(path_buffer, "\n")] = 0;
  while (path_error(path_buffer))
  {
    fgets(path_buffer, MAX_STR_LEN, stdin);
    path_buffer[strcspn(path_buffer, "\n")] = 0;
  }
}

void print_error(enum status code)
{
  switch (code)
  {
  case SUCCESS:
    break;
  case INVALID_PATH:
    printf("Invalid path");
    break;
  case INVALID_OPERATION:
    printf("Invalid operation");
    break;
  case NOT_FOUND:
    printf("Not found");
    break;
  case UNAVAILABLE:
    printf("Resource is busy");
    break;
  case READ_PERMISSION_DENIED:
    printf("Read permission denied");
    break;
  case WRITE_PERMISSION_DENIED:
    printf("Write permission denied");
    break;
  case CREATE_PERMISSION_DENIED:
    printf("Create permission denied");
    break;
  case DELETE_PERMISSION_DENIED:
    printf("Delete permission denied");
    break;
  case UNKNOWN_PERMISSION_DENIED:
    printf("Permission denied");
    break;
  case NON_EMPTY_DIRECTORY:
    printf("Directory is not empty");
    break;
  }
  printf("\n");
}

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

void fill_rd_path(const i32 i, const char *path, char *buf)
{
  sprintf(buf, ".rd%i/", i);
  strcat(buf, path);
}

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

int main()
{
  const i32 nm_sockfd = connect_to_port(NM_CLIENT_PORT);
  while (1)
  {
    const enum operation op = get_operation();
    SEND(nm_sockfd, op);
    enum status code;
    if (op == READ || op == WRITE || op == METADATA)
    {
      char path[MAX_STR_LEN];
      read_path(path);
      SEND(nm_sockfd, path);
      RECV(nm_sockfd, code);

      if (op == READ)
      {
        // to read from redundancy
        if (code == NOT_FOUND)
        {
          char rd1_path[MAX_STR_LEN] = {0};
          strcpy(rd1_path, ".rd1/");
          strcat(rd1_path, path);
          SEND(nm_sockfd, op);
          SEND(nm_sockfd, rd1_path);
          RECV(nm_sockfd, code);

          if (code == SUCCESS)
          {
            bzero(path, sizeof(path));
            strcpy(path, rd1_path);
          }
          else if (code == NOT_FOUND)
          {
            char rd2_path[MAX_STR_LEN] = {0};
            strcpy(rd2_path, ".rd2/");
            strcat(rd2_path, path);
            SEND(nm_sockfd, op);
            SEND(nm_sockfd, rd2_path);
            RECV(nm_sockfd, code);

            if (code == SUCCESS)
            {
              bzero(path, sizeof(path));
              strcpy(path, rd2_path);
            }
          }
        }
      }

      if (code != SUCCESS)
      {
        print_error(code);
        continue;
      }

      i32 port;
      CHECK(recv(nm_sockfd, &port, sizeof(port), 0), -1);
      const i32 ss_sockfd = connect_to_port(port);
      SEND(ss_sockfd, op);
      SEND(ss_sockfd, path);
      RECV(ss_sockfd, code);

      if (code != SUCCESS)
      {
        print_error(code);
        continue;
      }

      if (op == READ)
      {
        receive_and_print_file(ss_sockfd);
      }
      else if (op == WRITE)
      {
        char buffer[MAX_STR_LEN];
        fgets(buffer, MAX_STR_LEN, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        SEND(ss_sockfd, buffer);
      }
      else if (op == METADATA)
      {
        struct metadata meta;
        RECV(ss_sockfd, meta);
        printf("Size: %lu\n", meta.size);
        printf("Last accessed: %s\n", ctime(&meta.last_access_time));
        printf("Last modified: %s\n", ctime(&meta.last_modified_time));
        printf("Last status change: %s\n", ctime(&meta.last_status_change_time));
        print_mode(meta.mode);
        printf("\n");
      }
      close(ss_sockfd);
      enum operation ack = ACK;
      SEND(nm_sockfd, ack);
    }
    else if (op == CREATE_FILE || op == CREATE_FOLDER)
    {
      char path[MAX_STR_LEN];
      read_path(path);
      SEND(nm_sockfd, path);
      RECV(nm_sockfd, code);
      if (code != SUCCESS)
        print_error(code);
      else
        printf("Operation done successfully\n");
    }
    else if (op == DELETE_FILE || op == DELETE_FOLDER)
    {

      char path[MAX_STR_LEN];
      read_path(path);
      SEND(nm_sockfd, path);
      RECV(nm_sockfd, code);
      if (code != SUCCESS)
        print_error(code);
      else
      {
        printf("Operation done successfully\n");
        delete_rd_paths(nm_sockfd, op, path);
      }
    }
    else if (op == COPY_FILE || op == COPY_FOLDER)
    {
      char from_path[MAX_STR_LEN];
      char to_path[MAX_STR_LEN];
      read_path(from_path);
      read_path(to_path);

      SEND(nm_sockfd, from_path);
      SEND(nm_sockfd, to_path);

      RECV(nm_sockfd, code);
      if (code == SUCCESS)
        printf("Copied successfully\n");
      else
        print_error(code);
    }
    else
    {
      break;
    }
  }
  close(nm_sockfd);

  return 0;
}
