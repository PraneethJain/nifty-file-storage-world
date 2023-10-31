#include "headers.h"

const int MaxBufferLength = 50000;

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

const int MaxNameLength = 32;

struct Information
{
  char DirectoryName[MaxNameLength];
  u8 NumChild;
  /*
  Pointer to files in directory.
  */
};

struct TreeNode
{
  struct Information NodeInfo;
  struct TreeNode *NextSibling;
  struct TreeNode *ChildDirectoryLL; // LL - > Linked List
};

typedef struct TreeNode *Tree;

struct TreeNode *InitNode(char *Name)
{
  struct TreeNode *Node = malloc(sizeof(struct TreeNode));
  strcpy(Node->NodeInfo.DirectoryName, Name);
  Node->NodeInfo.NumChild = 0;
  Node->ChildDirectoryLL = NULL;
  Node->NextSibling = NULL;
}

Tree InitTree()
{
  return InitNode(".");
}

struct TreeNode *FindChild(Tree T, const char *ChildName, int CreateFlag)
{
  if (T->ChildDirectoryLL == NULL)
  {
    if (CreateFlag)
    {
      T->ChildDirectoryLL = InitNode(ChildName);
      return T->ChildDirectoryLL;
    }
    return NULL;
  }

  struct TreeNode *trav = T->ChildDirectoryLL;

  while (trav->NextSibling != NULL)
  {
    if (strcmp(trav->NodeInfo.DirectoryName, ChildName) == 0)
    {
      return trav;
    }
    trav = trav->NextSibling;
  }

  if (strcmp(trav->NodeInfo.DirectoryName, ChildName) == 0)
  {
    return trav;
  }

  if (CreateFlag)
  {
    T->ChildDirectoryLL = InitNode(ChildName);
    return T->ChildDirectoryLL;
  }
  return NULL;
}

Tree FindNode(Tree T)
{

}

Tree InsertNode(Tree T, struct TreeNode *Target)
{
}

int SendTreeDataDriver(Tree T, char buffer[MaxBufferLength], int* lastindex)
{
  buffer[*lastindex] = '/';
  *lastindex++;

  CHECK(strcpy(&buffer[*lastindex], T->NodeInfo.DirectoryName), -1);
  *lastindex += strlen(T->NodeInfo.DirectoryName);

  struct TreeNode* trav = T->ChildDirectoryLL;

  while (trav != NULL)
  {
    SendTreeDataDriver(trav, buffer, lastindex);
    trav = trav->NextSibling;
  }

  CHECK(strcpy(&buffer[*lastindex], "/.."), -1);
  *lastindex += 3;

  return 0;
}

char* SendTreeData(Tree T)
{
  char buffer[MaxBufferLength];
  int lastindex = 0;
  SendTreeDataDriver(T, buffer, &lastindex);
  return buffer;
}

int ReceiveTreeDataDriver(Tree T, char buffer[MaxBufferLength], int* lastindex)
{
  
}

Tree ReceiveTreeData(char buffer[MaxBufferLength])
{
  Tree T = InitTree();
  int lastindex = 0;
  ReceiveTreeDataDriver(T, buffer, &lastindex);
  return T;
}