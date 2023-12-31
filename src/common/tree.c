/**
 * @file tree.c
 * @brief Contains all the functions related to directory trees.
 * @details
 *    - Functions for initialising node for info for each directory.
 *    - Functions for finding directory nodes with paths.
 *    - Functions for utilising the nodes or finding specific info.
 */

#include "headers.h"

const i32 MaxBufferLength = MAX_STR_LEN * 2000;

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
  pthread_rwlock_init(&Node->NodeInfo.rwlock, NULL);
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

i32 SendTreeDataDriver(Tree T, char *buffer, u32 *lastindex, u32 BufferCapacity)
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

i32 SendTreeData(Tree T, char *buffer)
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
  if (strstr(T->NodeInfo.DirectoryName, ".rd") == T->NodeInfo.DirectoryName)
    return;
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

void GetPrintedSubtreeDriver(Tree T, char *printedtree, u32 indent)
{
  if (strstr(T->NodeInfo.DirectoryName, ".rd") == T->NodeInfo.DirectoryName)
    return;
  for (u32 i = 0; i < indent; i++)
  {
    strcat(printedtree, "\t");
  }
  if (T->NodeInfo.Access == 0)
    strcat(printedtree, C_BLACK);
  else if (T->NodeInfo.IsFile)
    strcat(printedtree, C_BLUE);
  else
    strcat(printedtree, C_YELLOW);
  strcat(printedtree, T->NodeInfo.DirectoryName);
  strcat(printedtree, "\n");
  strcat(printedtree, C_RESET);
  struct TreeNode *trav = T->ChildDirectoryLL;
  while (trav != NULL)
  {
    GetPrintedSubtreeDriver(trav, printedtree, indent + 1);
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

/**
 * @brief finds the tree node with the given path
 *
 * @param DirPath the path of the required node relative to T
 * @param T the root node of the tree
 * @param CreateFlag if the required node doesn't exist and CreateFlag is enabled
 * then the node is created. Otherwise NULL is returned
 * @return struct TreeNode* Required Node
 */
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

void GetPrintedSubtree(Tree T, const char *path, char *printedtree)
{
  Tree temp = ProcessDirPath(path, T, 0);
  GetPrintedSubtreeDriver(temp, printedtree, 0);
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

void InitDirectory(Tree Parent)
{
  Parent->NodeInfo.IsFile = 0;
  Parent->NodeInfo.Access = 1;
  ProcessWholeDir(".", Parent);
}

void RemoveInaccessiblePath(Tree Parent, const char *DirPath)
{
  Tree T = ProcessDirPath(DirPath, Parent, 0);
  if (T == NULL)
    return;
  DeleteTree(T);
}

/**
 * @brief Merge Tree T1 with T2.
 * 
 * @param T1 
 * @param T2 
 * @param ss_id 
 * @param UUID 
 */
void MergeTree(Tree T1, Tree T2, u32 ss_id, char *UUID)
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
    strcpy(trav->NodeInfo.UUID, UUID);
    trav->Parent = T1;
    trav = trav->NextSibling;
  }
  free(T2);
}

/**
 * @brief Deletes all cache nodes with ssid of disconnected storage server
 * 
 * @param ss_id 
 */
void DeleteFromCacheWithSSID(u32 ss_id)
{
  node *prev = NULL;
  node *curr = cache_head.ll;
  while (curr != NULL)
  {
    if (curr->SSID == (i32)ss_id)
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

/**
 * @brief Remove all the directory nodes from the tree of server with the given ssid.
 * 
 * @param T 
 * @param ss_id 
 */
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

/**
 * @brief Checks if path is cached
 * 
 * @param path 
 * @return i32 
 */
i32 CheckCache(const char *path)
{
  i32 req_ssid;
  node *prev = NULL;
  node *curr = cache_head.ll;
  while (curr != NULL)
  {
    if (strcmp(curr->path, path) == 0)
    {
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
  return -1;
}

/**
 * @brief Inserts path into cache
 * 
 * @param path 
 * @param ssid 
 */
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

/**
 * @brief Get the SSID of the SS from the path.
 * 
 * @param T 
 * @param path 
 * @param cache_flag 
 * @return i32 
 */
i32 GetPathSSID(Tree T, const char *path, bool cache_flag)
{
  if (strncmp(path, ".rd", 3) != 0)
  {
    i32 req_ssid = CheckCache(path);
    if (req_ssid != -1)
      return req_ssid;
  }
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
  if (cache_flag)
    InsertIntoCache(path, RetT->NodeInfo.ss_id);
  return RetT->NodeInfo.ss_id;
}

char *GetParent(const char *path)
{
  char path_copy[MAX_STR_LEN];
  strcpy(path_copy, path);
  i32 string_length = strlen(path);
  i32 flag = 1;
  i32 i = string_length - 1;
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

void AddFile(Tree T, const char *path, i32 port_ss_nm, char *UUID)
{
  Tree temp = ProcessDirPath(path, T, 1);
  temp->NodeInfo.Access = 1;
  temp->NodeInfo.IsFile = 1;
  temp->NodeInfo.ss_id = port_ss_nm;
  strcpy(temp->NodeInfo.UUID, UUID);
}

void AddFolder(Tree T, const char *path, i32 port_ss_nm, char *UUID)
{
  Tree temp = ProcessDirPath(path, T, 1);
  temp->NodeInfo.Access = 1;
  temp->NodeInfo.IsFile = 0;
  temp->NodeInfo.ss_id = port_ss_nm;
  strcpy(temp->NodeInfo.UUID, UUID);
}

/**
 * @brief Deletes node with given path from cache
 * 
 * @param path 
 */
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

/**
 * @brief Get the Tree From Path String
 *
 * @param T
 * @param path
 * @return Tree
 */
Tree GetTreeFromPath(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  return temp;
}

/**
 * @brief Checks if the current path is empty or directory or a file.
 *
 * @param T The root node of the tree.
 * @param path The path relative to the root node.
 * @return i8 -1 if the file/dir doesn't exist, 0 if it is a directory
 * and 1 if it is a file.
 */
i8 IsFile(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  if (temp != NULL)
    return temp->NodeInfo.IsFile;
  return -1;
}

/**
 * @brief Check if from_path directory is ancestor of to_path.
 * 
 * @param T 
 * @param from_path 
 * @param to_path 
 * @return 1 if from_path is ancestor of to_path and 0 otherwise.
 */
i8 Ancestor(Tree T, const char *from_path, const char *to_path)
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

void AcquireReaderLockDriver(Tree T)
{
  pthread_rwlock_rdlock(&T->NodeInfo.rwlock);
  for (Tree trav = T->ChildDirectoryLL; trav != NULL; trav = trav->NextSibling)
  {
    AcquireReaderLockDriver(trav);
  }
}

/**
 * @brief Acquire reader lock of subtree of directory with the given path.
 * 
 * @param T 
 * @param path 
 */
void AcquireReaderLock(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  if (temp == NULL)
    return;
  AcquireReaderLockDriver(temp);
}

void AcquireWriterLockDriver(Tree T)
{
  pthread_rwlock_wrlock(&T->NodeInfo.rwlock);
  for (Tree trav = T->ChildDirectoryLL; trav != NULL; trav = trav->NextSibling)
  {
    AcquireWriterLockDriver(trav);
  }
}

/**
 * @brief Acquire writer lock of subtree of directory with the given path.
 * 
 * @param T 
 * @param path 
 */
void AcquireWriterLock(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  if (temp == NULL)
    return;
  AcquireWriterLockDriver(temp);
}

void ReleaseLockDriver(Tree T)
{
  pthread_rwlock_unlock(&T->NodeInfo.rwlock);
  for (Tree trav = T->ChildDirectoryLL; trav != NULL; trav = trav->NextSibling)
  {
    ReleaseLockDriver(trav);
  }
}

/**
 * @brief Release lock of subtree of directory with the given path.
 * 
 * @param T 
 * @param path 
 */
void ReleaseLock(Tree T, const char *path)
{
  Tree temp = ProcessDirPath(path, T, 0);
  if (temp == NULL)
    return;
  ReleaseLockDriver(temp);
}
