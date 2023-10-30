#include "../common/headers.h"
#include "headers.h"

i32 connect_to_nm()
{
  const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(sockfd, -1);

  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(NM_CLIENT_PORT);
  addr.sin_addr.s_addr = inet_addr(LOCALHOST);
  CHECK(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)), -1);

  return sockfd;
}

i8 get_operation()
{
  printf("Functionalities:-\n1.Read\n2.Write\n3.Metadata\n4.Create file\n5.Delete file\n6.Create folder\n7.Delete "
         "folder\n8.Copy file\n9.Copy folder\n");
  printf("Enter the number of the functionality that you wish to perform: ");
  i8 op_int;
  scanf("%hhi", &op_int);
  if (op_int < 1 || op_int > 9)
    return -1;
  return op_int;
}

int main()
{
  const i32 nm_sockfd = connect_to_nm();

  while (1)
  {
    i8 op_int = get_operation();
    if (op_int == -1)
    {
      printf("Enter valid choice!\n");
      continue;
    }
    enum operation op = op_int;
    CHECK(send(nm_sockfd, &op, sizeof(op), 0), -1);
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
