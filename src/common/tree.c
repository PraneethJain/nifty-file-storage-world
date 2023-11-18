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

typedef struct node
{
  char path[MAX_STR_LEN];
  i32 SSID;
  struct node *next;
} node;

struct
{
  node *ll;
  i32 length;
} cache_head = {0};

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
  if (ChildName == NULL)
  {
    return T;
  }
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
  buffer[lastindex] = '\0';
  return 0;
}

i32 ReceiveTreeDataDriver(Tree T, char *buffer, u32 *lastindex, u32 BufferCapacity)
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

struct TreeNode *ProcessDirPath(const char *DirPath, Tree T, bool CreateFlag)
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
    if (temp == NULL && CreateFlag)
    {
      temp = FindChild(Cur, token, CreateFlag, 0);
      temp->NodeInfo.Access = 0;
    }
    Cur = temp;
    if (Cur == NULL)
      return NULL;
    token = strtok(NULL, Delim);
  }
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
      struct TreeNode *T = ProcessDirPath(en->d_name, Parent, 1);
      T->NodeInfo.IsFile = 0;
      T->NodeInfo.Access = 1;
      ProcessWholeDir(filepath, T);
    }
    else
    {
      struct TreeNode *T = ProcessDirPath(en->d_name, Parent, 1);
      T->NodeInfo.Access = 1;
      T->NodeInfo.IsFile = 1;
    }
    free(files[index]);
    index++;
  }
  free(files);
}

bool CheckIfFile(char *DirPath)
{
  struct stat stats;
  stat(DirPath, &stats);

  if (S_ISDIR(stats.st_mode))
    return 0;

  return 1;
}

void AddAccessibleDir(char *DirPath, Tree Parent)
{
  struct TreeNode *T = ProcessDirPath(DirPath, Parent, 1);
  if (CheckIfFile(DirPath))
  {
    T->NodeInfo.IsFile = 1;
    T->NodeInfo.Access = 1;
    return;
  }

  T->NodeInfo.IsFile = 0;
  T->NodeInfo.Access = 1;
  ProcessWholeDir(DirPath, T);
}

void MergeTree(Tree T1, Tree T2, u32 ss_id)
{
  Tree trav = T1->ChildDirectoryLL;

  if (trav != NULL)
  {
    while (trav->NextSibling != NULL)
    {
      trav = trav->NextSibling;
    }
    trav->NextSibling = T2->ChildDirectoryLL;
  }
  else
  {
    T1->ChildDirectoryLL = T2->ChildDirectoryLL;
  }

  if (T2->ChildDirectoryLL != NULL)
    T2->ChildDirectoryLL->PrevSibling = trav;
  trav = T2->ChildDirectoryLL;
  while (trav != NULL)
  {
    trav->NodeInfo.ss_id = ss_id;
    trav->Parent = T1;
    trav = trav->NextSibling;
  }
  free(T2);
}

//  Deletes all cache nodes with ssid of disconnected storage server
void DeleteFromCacheWithSSID(u32 ss_id)
{
  node *prev = NULL;
  node *curr = cache_head.ll;
  while (curr != NULL)
  {
    if (curr->SSID == (i32)ss_id) // typecasting because of moida's nakhre
    {
      node *temp = curr;
      if (prev != NULL)
        prev->next = curr->next;
      else
        cache_head.ll = curr->next;
      curr = curr->next;
      free(temp);
      --cache_head.length;
      continue;
    }
    prev = curr;
    curr = curr->next;
  }
}

void RemoveServerPath(Tree T, u32 ss_id)
{
  DeleteFromCacheWithSSID(ss_id);
  Tree trav = T->ChildDirectoryLL;
  while (trav != NULL)
  {
    if (trav->NodeInfo.ss_id == ss_id)
    {
      if (trav->NextSibling == NULL)
      {
        DeleteTree(trav);
        return;
      }
      trav = trav->NextSibling;
      DeleteTree(trav->PrevSibling);
    }
    else
    {
      trav = trav->NextSibling;
    }
  }
}

// checks if path is cached
i32 CheckCache(const char *path)
{
  i32 req_ssid;
  node *prev = NULL;
  node *curr = cache_head.ll;
  printf("Length of cache = %d\n", cache_head.length); // remove if not needed
  while (curr != NULL)
  {
    if (strcmp(curr->path, path) == 0)
    {
      printf("Cache hit!\n");
      req_ssid = curr->SSID;
      if (prev != NULL)
      {
        prev->next = curr->next;
        curr->next = cache_head.ll;
        cache_head.ll = curr;
      }
      return req_ssid;
    }
    prev = curr;
    curr = curr->next;
  }
  printf("Cache miss!\n");
  return -1;
}

// inserts path into cache
void InsertIntoCache(const char *path, i32 ssid)
{
  node *newnode = malloc(sizeof(node));
  strcpy(newnode->path, path);
  newnode->SSID = ssid;
  newnode->next = cache_head.ll;
  cache_head.ll = newnode;
  if (cache_head.length == CACHE_SIZE)
  {
    node *curr = cache_head.ll;
    // assuming max cache size is > 2
    while (curr->next->next != NULL)
    {
      curr = curr->next;
    }
    free(curr->next);
    curr->next = NULL;
  }
  else
    ++cache_head.length;
}

i32 GetPathSSID(Tree T, const char *path)
{
  i32 req_ssid = CheckCache(path);
  if (req_ssid != -1)
    return req_ssid;
  char pathcopy[MAX_STR_LEN];
  Tree temp = ProcessDirPath(path, T, 0);
  if (temp == NULL || temp->NodeInfo.Access == 0)
    return -1;
  strcpy(pathcopy, path);
  char *Delim = "/\\";
  char *token = strtok(pathcopy, Delim);
  Tree RetT = FindChild(T, token, 0, 0);
  if (RetT == NULL)
    return -1;
  InsertIntoCache(path, RetT->NodeInfo.ss_id);
  return RetT->NodeInfo.ss_id;
}

Tree GetParentOfPath(Tree T, const char *path)
{
  char pathcopy[MAX_STR_LEN];
  strcpy(pathcopy, path);
  char *Delim = "/\\";

  Tree Cur = T;

  char *token = strtok(pathcopy, Delim);
  if (token == NULL)
  {
    return 0;
  }
  char *nexttoken = strtok(NULL, Delim);
  while (nexttoken != NULL)
  {
    Cur = FindChild(Cur, token, 0, 0);
    if (Cur == NULL)
      return NULL;
    strcpy(token, nexttoken);
    nexttoken = strtok(NULL, Delim);
  }
  return Cur;
}

char *get_parent(const char *path)
{
  char path_copy[MAX_STR_LEN];
  strcpy(path_copy, path);
  int string_length = strlen(path);
  int flag = 1;
  int i = string_length - 1;
  for (; i >= 0; i--)
  {
    if ((path[i] != '/' && path[i] != '\\'))
    {
      flag = 0;
    }
    else if (!flag)
    {
      while (i >= 0 && (path[i] == '/' || path[i] == '\\'))
      {
        i--;
      }
      i++;
      char *ret_string = malloc(sizeof(char) * MAX_STR_LEN);
      path_copy[i] = '\0';
      strcpy(ret_string, path_copy);
      return (ret_string);
    }
  }
  return NULL;
}

void AddFile(Tree T, const char *path, i32 port_ss_nm)
{
  Tree temp = ProcessDirPath(path, T, 1);
  temp->NodeInfo.Access = 1;
  temp->NodeInfo.IsFile = 1;
  temp->NodeInfo.ss_id = port_ss_nm;
}

void AddFolder(Tree T, const char *path, i32 port_ss_nm)
{
  Tree temp = ProcessDirPath(path, T, 1);
  temp->NodeInfo.Access = 1;
  temp->NodeInfo.IsFile = 0;
  temp->NodeInfo.ss_id = port_ss_nm;
}

void DeleteFromCache(const char *path)
{
  node *prev = NULL;
  node *curr = cache_head.ll;
  while (curr != NULL)
  {
    if (strcmp(curr->path, path) == 0)
    {
      if (prev == NULL)
      {
        cache_head.ll = curr->next;
        curr->next = NULL;
      }
      else
        prev->next = curr->next;
      cache_head.length--;
      free(curr);
      break;
    }
    prev = curr;
    curr = curr->next;
  }
}

void DeleteFile(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  DeleteTree(temp);
  DeleteFromCache(path);
}

void DeleteFolder(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  DeleteTree(temp);
  DeleteFromCache(path);
}

Tree GetTreeFromPath(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  return temp;
}

i8 Is_File(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  if (temp != NULL)
    return temp->NodeInfo.IsFile;
  return -1;
}

i8 ancestor(Tree T, const char *from_path, const char *to_path)
{
  Tree to_node = ProcessDirPath(to_path, T, 0);
  Tree from_node = ProcessDirPath(from_path, T, 0);
  while (to_node != NULL)
  {
    if (from_node == to_node)
    {
      return 1;
    }
    to_node = to_node->Parent;
  }
  return 0;
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
