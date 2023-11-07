#include "headers.h"

const i32 MaxBufferLength = 50000;

/*
      *
     /|\
    / | \
   A  B  C(Can be stored in any server)

*--->A->B->C->NULL
     |  |  |
     |  |  |
     v  v  v
    A/D
*/

struct TreeNode *InitNode(const char *Name, struct TreeNode *Parent)
{
  struct TreeNode *Node = malloc(sizeof(struct TreeNode));
  strcpy(Node->NodeInfo.DirectoryName, Name);
  Node->NodeInfo.NumChild = 0;
  Node->Parent = Parent;
  Node->ChildDirectoryLL = NULL;
  Node->NextSibling = NULL;
  Node->PrevSibling = NULL;
  if (Parent != NULL)
  {
    Parent->NodeInfo.NumChild++;
  }
  return Node;
}

Tree InitTree()
{
  return InitNode(".", NULL);
}

/*
Finds a child node for a given TreeNode T.

If CreateFlag is enabled then a child node with the given name will be created if
child doesn't exist. The function will return this node.

If NoNameFlag is enabled then a child node will be created with no name.
The function will return this node.
*/
struct TreeNode *FindChild(Tree T, const char *ChildName, bool CreateFlag, bool NoNameFlag)
{
  if (T->ChildDirectoryLL == NULL)
  {
    if (CreateFlag)
    {
      T->ChildDirectoryLL = InitNode(ChildName, T);
      return T->ChildDirectoryLL;
    }
    return NULL;
  }

  struct TreeNode *trav = T->ChildDirectoryLL;

  while (trav->NextSibling != NULL)
  {
    if (!NoNameFlag && strcmp(trav->NodeInfo.DirectoryName, ChildName) == 0)
    {
      return trav;
    }
    trav = trav->NextSibling;
  }

  if (!NoNameFlag && strcmp(trav->NodeInfo.DirectoryName, ChildName) == 0)
  {
    return trav;
  }

  if (CreateFlag)
  {
    trav->NextSibling = InitNode(ChildName, T);
    trav->NextSibling->PrevSibling = trav;
    return trav->NextSibling;
  }
  return NULL;
}

#define DIRINFO 'D'
#define DIREND '.'

int SendTreeDataDriver(Tree T, char *buffer, u32 *lastindex, u32 BufferCapacity)
{
  if (*lastindex >= BufferCapacity)
    return -1;

  buffer[*lastindex] = DIRINFO;
  *lastindex += 1;

  if (*lastindex + sizeof(T->NodeInfo) >= BufferCapacity)
    return -1;

  memcpy(&buffer[*lastindex], &T->NodeInfo, sizeof(T->NodeInfo));
  *lastindex += sizeof(T->NodeInfo);

  struct TreeNode *trav = T->ChildDirectoryLL;

  while (trav != NULL)
  {
    if (SendTreeDataDriver(trav, buffer, lastindex, BufferCapacity) == -1)
      return -1;
    trav = trav->NextSibling;
  }

  if (*lastindex >= BufferCapacity)
    return -1;

  buffer[*lastindex] = DIREND;
  *lastindex += 1;

  return 0;
}

int SendTreeData(Tree T, char *buffer)
{
  u32 lastindex = 0;
  if (SendTreeDataDriver(T, buffer, &lastindex, MaxBufferLength) == -1)
    return -1;
  return 0;
}

i32 ReceiveTreeDataDriver(Tree T, char* buffer, u32 *lastindex, u32 BufferCapacity)
{
  if (*lastindex + sizeof(T->NodeInfo) >= BufferCapacity)
    return -1;
  memcpy(&T->NodeInfo, &buffer[*lastindex], sizeof(T->NodeInfo));
  *lastindex += sizeof(T->NodeInfo);

  T->NodeInfo.NumChild = 0;
  while (buffer[*lastindex] == DIRINFO)
  {
    *lastindex += 1;
    if (*lastindex >= BufferCapacity)
      return -1;

    struct TreeNode *Node = FindChild(T, "", 1, 1);

    if (ReceiveTreeDataDriver(Node, buffer, lastindex, BufferCapacity) == -1)
      return -1;
  }
  *lastindex += 1;
  return 0;
}

Tree ReceiveTreeData(char *buffer)
{
  Tree T = InitTree();
  u32 lastindex = 1;
  if (ReceiveTreeDataDriver(T, buffer, &lastindex, MaxBufferLength) == -1)
    return NULL;
  return T;
}

void PrintTree(Tree T, u32 indent)
{
  for (u32 i = 0; i < indent; i++)
  {
    printf("\t");
  }
  if (T->NodeInfo.Access == 0)
    printf(C_BLACK);
  else if (T->NodeInfo.IsFile)
    printf(C_BLUE);
  else
    printf(C_YELLOW);
  printf("%s\n", T->NodeInfo.DirectoryName);
  printf(C_RESET);
  struct TreeNode *trav = T->ChildDirectoryLL;
  while (trav != NULL)
  {
    PrintTree(trav, indent + 1);
    trav = trav->NextSibling;
  }
}

i32 DeleteTree(Tree T)
{
  struct TreeNode *trav = T->ChildDirectoryLL;
  struct TreeNode *next;

  while (trav != NULL)
  {
    next = trav->NextSibling;
    DeleteTree(trav);
    trav = next;
  }

  if (T->Parent != NULL)
  {
    T->Parent->NodeInfo.NumChild--;
    if (T->Parent->ChildDirectoryLL == T)
    {
      T->Parent->ChildDirectoryLL = T->NextSibling;
    }
  }

  if (T->NextSibling != NULL)
    T->NextSibling->PrevSibling = T->PrevSibling;

  if (T->PrevSibling != NULL)
    T->PrevSibling->NextSibling = T->NextSibling;

  free(T);
  return 0;
}

struct TreeNode *ProcessDirPath(char *DirPath, Tree T, bool CreateFlag)
{
  struct TreeNode *Cur = T;
  if (T == NULL)
    return NULL;
  char DirPathCopy[MAX_STR_LEN];
  strcpy(DirPathCopy, DirPath);
  char *Delim = "/\\";
  char *token = strtok(DirPathCopy, Delim);
  while (token != NULL)
  {
    Tree temp = FindChild(Cur, token, 0, 0);
    if (temp == NULL && CreateFlag) {
      temp = FindChild(Cur, token, CreateFlag, 0);
      temp->NodeInfo.Access = 0;
    }
    Cur = temp;
    if (Cur == NULL)
      return NULL;
    token = strtok(NULL, Delim);
  }
  Cur->NodeInfo.Access = 1;
  return Cur;
}

bool IsDirectory(const char *location)
{
    struct stat st;
    i32 status = lstat(location, &st);
    return S_ISDIR(st.st_mode) && (status != -1);
}

void ProcessWholeDir(char *DirPath, Tree Parent)
{
  struct dirent *en;
  struct dirent **files;
  i32 numfiles = scandir(DirPath, &files, NULL, NULL);
  if (numfiles < 0)
  {
    fprintf(stderr, "Unable to execute scandir on directory: %s\n", DirPath);
    return;
  }
  i32 index = 0;
  while (index < numfiles)
  {
    en = files[index];
    if (en->d_name[0] == '.' && ((en->d_name[1] == '.' && en->d_name[2] == '\0') || en->d_name[1] == '\0'))
    {
      free(files[index]);
      index++;
      continue;
    }
    char filepath[MAX_STR_LEN];
    strcpy(filepath, DirPath);
    strcat(filepath, "/");
    strcat(filepath, en->d_name);
    if (IsDirectory(filepath))
    {
      struct TreeNode* T = ProcessDirPath(en->d_name, Parent, 1);
      T->NodeInfo.IsFile = 0;
      ProcessWholeDir(filepath, T);
    }
    else
    {
      struct TreeNode* T = ProcessDirPath(en->d_name, Parent, 1);
      T->NodeInfo.IsFile = 1;
    }
    free(files[index]);
    index++;
  }
  free(files);
}

void AddAccessibleDir(char *DirPath, Tree Parent) {
  struct TreeNode* T = ProcessDirPath(DirPath, Parent, 1);
  ProcessWholeDir(DirPath, T);
}

void MergeTree(Tree T1, Tree T2, u32 ss_id) {
  Tree trav = T2->ChildDirectoryLL;
  while (trav != NULL) {
    Tree temp = FindChild(T1, "", 1, 1);
    memcpy(&temp->NodeInfo, &trav->NodeInfo, sizeof(trav->NodeInfo));
    temp->NodeInfo.ss_id = ss_id;
    Tree next = trav->NextSibling;
    free(trav);
    trav = next;
  }
  free(T2);
}

// void RandomTest()
// {
//   Tree T = InitTree();
//   Tree T2 = FindChild(T, "Hello", 1, 0);
//   Tree T3 = FindChild(T2, "Fuck YOU", 1, 0);
//   Tree T4 = FindChild(T, "LOL", 1, 0);
//   Tree T5 = FindChild(T3, "NOPLEASE", 1, 0);
//   Tree T6 = FindChild(T4, "OAWKFEOWAJFAW", 1, 0);
//   Tree T7 = ProcessDirPath("Hello/My/Name/Is/Harshvardhan", T, 1);
//   char Buffer[MaxBufferLength];
//   SendTreeData(T, Buffer);
//   Tree TRec = ReceiveTreeData(Buffer);
//   DeleteTree(ProcessDirPath("Hello/My/Name", T, 0));
//   printf("OG:\n");
//   PrintTree(T, 0);
//   printf("Test:\n");
//   PrintTree(TRec, 0);
//   if ((T7 = ProcessDirPath("Hello/My/Name//Is/Harshvardhan", TRec, 0)) != NULL)
//   {
//     printf("Passed: %s!\n", T7->NodeInfo.DirectoryName);
//   }
//   else
//   {
//     printf("Failed\n");
//   }
// }
