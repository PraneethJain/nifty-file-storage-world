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
  printf("Operations:-\n1.Read\n2.Write\n3.Metadata\n4.Create file\n5.Delete file\n6.Create folder\n7.Delete "
         "folder\n8.Copy file\n9.Copy folder\n10. Exit\n");
  i8 op_int = -1;
  while (op_int < 1 || op_int > 10)
  {
    printf("Enter the number of the operation that you wish to perform: ");
    scanf("%hhi", &op_int);
  }
  return op_int - 1;
}

int main()
{
  const i32 nm_sockfd = connect_to_port(NM_CLIENT_PORT);
  while (1)
  {
    const enum operation op = get_operation();
    CHECK(send(nm_sockfd, &op, sizeof(op), 0), -1);
    if (op == READ || op == WRITE || op == METADATA)
    {
      char path[MAX_STR_LEN];
      scanf("%s", path);
      // error handle
      CHECK(send(nm_sockfd, path, sizeof(path), 0), -1);
      i32 port;
      CHECK(recv(nm_sockfd, &port, sizeof(port), 0), -1);
      printf("%i\n", port);
      // connect to ss and do your operations
      const i32 ss_sockfd = connect_to_port(port);
      CHECK(send(ss_sockfd, path, sizeof(path), 0), -1);
      close(ss_sockfd);
    }
    else if (op == CREATE_FILE || op == DELETE_FILE || op == CREATE_FOLDER || op == DELETE_FOLDER)
    {
      char path[MAX_STR_LEN];
      scanf("%s", path);
      // error handle
      CHECK(send(nm_sockfd, path, sizeof(path), 0), -1);
      i32 status;
      CHECK(recv(nm_sockfd, &status, sizeof(status), 0), -1);
      printf("status: %i\n", status);
    }
    else if (op == COPY_FILE || op == COPY_FOLDER)
    {
      char from_path[MAX_STR_LEN];
      char to_path[MAX_STR_LEN];
      scanf("%s", from_path);
      scanf("%s", to_path);
      // error handle
      CHECK(send(nm_sockfd, from_path, sizeof(from_path), 0), -1);
      CHECK(send(nm_sockfd, to_path, sizeof(to_path), 0), -1);
      i32 status;
      CHECK(recv(nm_sockfd, &status, sizeof(status), 0), -1);
      printf("status: %i\n", status);
    }
    else
    {
      break;
    }
  }
  close(nm_sockfd);

  return 0;
}
