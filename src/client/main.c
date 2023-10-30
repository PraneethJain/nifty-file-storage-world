#include "../common/headers.h"
#include "headers.h"

void send_to_nm(const void *buf, const size_t size)
{
  const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(sockfd, -1);

  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(NM_CLIENT_PORT);
  addr.sin_addr.s_addr = inet_addr(LOCALHOST);
  CHECK(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)), -1);

  CHECK(send(sockfd, buf, size, 0), -1);
  CHECK(close(sockfd), -1);
}

void recv_from_nm(void *buf, const size_t size)
{
  const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(sockfd, -1);

  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(NM_CLIENT_PORT);
  addr.sin_addr.s_addr = inet_addr(LOCALHOST);
  CHECK(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)), -1);

  CHECK(recv(sockfd, buf, size, 0), -1);
  CHECK(close(sockfd), -1);
}

i8 get_operation()
{
  printf("Operations:-\n1.Read\n2.Write\n3.Metadata\n4.Create file\n5.Delete file\n6.Create folder\n7.Delete "
         "folder\n8.Copy file\n9.Copy folder\n");
  i8 op_int = -1;
  while (op_int < 1 || op_int > 9)
  {
    printf("Enter the number of the operation that you wish to perform: ");
    scanf("%hhi", &op_int);
  }
  return op_int - 1;
}

int main()
{
  while (1)
  {
    enum operation op = get_operation();
    send_to_nm(&op, sizeof(op));
    if (op == READ || op == WRITE || op == METADATA)
    {
      char path[MAX_STR_LEN];
      scanf("%s", path);
      // error handle
      send_to_nm(path, sizeof(path));
      i32 port;
      recv_from_nm(&port, sizeof(port));
      printf("%d\n", port);
    }
  }
  // if (op == COPY_FILE)
  // {
  //   char src_path[100];
  //   char dst_path[100];
  //   printf("Enter the source path: ");
  //   scanf("%s", src_path);
  //   // handle error
  //   printf("Enter the destination path: ");
  //   scanf("%s", dst_path);
  //   // handle error
  //   char request[200];
  //   // snprintf(request, 200, "%s %s %s", , src_path, dst_path);
  // }
  // else if (op >= 1 && op <= 6)
  // {
  //   char path[100];
  //   printf("Enter the path of the file/folder: ");
  //   scanf("%s", path);
  //   char request[200];
  //   // snprintf(request, 200, "%s %s", , path);
  // }
  // else
  // {
  //   printf("Invalid choice\n");
  // }

  return 0;
}
