#include "../common/headers.h"
#include "headers.h"

int main()
{
  char functionality[10];
  printf("Functionalities:-\n1.Read\n2.Write\n3.Retrieve information\n4.Create\n5.Delete\n6.Copy\n");
  printf("Enter the number of the functionality that you wish to perform: ");
  int choice;
  scanf("%d", &choice);
  if (choice == 6)
  {
    char src_path[100];
    char dst_path[100];
    printf("Enter the source path: ");
    scanf("%s", src_path);
    // handle error
    printf("Enter the destination path: ");
    scanf("%s", dst_path);
    // handle error
    char request[200];
    snprintf(request, 200, "%s %s %s", enum, src_path, dst_path);
  }
  else if (choice >= 1 && choice <= 6)
  {
    char path[100];
    printf("Enter the path of the file/folder: ");
    scanf("%s", path);
    char request[200];
    snprintf(request, 200, "%s %s", enum, path);
  }
  else
  {
    printf("Invalid choice\n");
  }

  // send the request to the NM

  return 0;
}
