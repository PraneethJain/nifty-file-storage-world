/**
 * @file main.c
 * @brief Entry point for a client
 * @details
 * - Command line interface for a client
 * - Reads operations and sends to the naming server and storage servers
 */

#include "../common/headers.h"
#include "headers.h"

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
        for (int i = 1; i <= 3 && code == NOT_FOUND; ++i)
        {
          char rdi_path[MAX_STR_LEN];
          fill_rd_path(i, path, rdi_path);
          SEND(nm_sockfd, op);
          SEND(nm_sockfd, rdi_path);
          RECV(nm_sockfd, code);

          if (code != SUCCESS)
            continue;

          bzero(path, sizeof(path));
          strcpy(path, rdi_path);
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
        print_metadata(meta);
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
        printf(C_GREEN "Operation done successfully\n" C_RESET);
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
        printf(C_GREEN "Operation done successfully\n" C_RESET);
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
    else if (op == PRINT_TREE)
    {
      char path_of_subdir[MAX_STR_LEN];
      read_path(path_of_subdir);

      SEND(nm_sockfd, path_of_subdir);

      RECV(nm_sockfd, code);
      if (code == SUCCESS)
      {
        char printed_tree[200 * MAX_STR_LEN];
        receive_data_in_packets(printed_tree, nm_sockfd, sizeof(printed_tree));
        printf("%s", printed_tree);
      }
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
