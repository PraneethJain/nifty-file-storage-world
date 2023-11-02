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
  char DirectoryName[32];
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
  struct TreeNode *Parent;
};

typedef struct TreeNode *Tree;

struct TreeNode *InitNode(const char *Name, struct TreeNode* Parent)
{
  struct TreeNode *Node = malloc(sizeof(struct TreeNode));
  strcpy(Node->NodeInfo.DirectoryName, Name);
  Node->NodeInfo.NumChild = 0;
  Node->Parent = Parent;
  Node->ChildDirectoryLL = NULL;
  Node->NextSibling = NULL;
  return Node;
}

Tree InitTree()
{
  return InitNode(".", NULL);
}

struct TreeNode *FindChild(Tree T, const char *ChildName, int CreateFlag, int NoNameFlag)
{
  if (T->ChildDirectoryLL == NULL)
  {
    if (CreateFlag)
    {
      T->ChildDirectoryLL = InitNode(ChildName, T);
      T->NodeInfo.NumChild++;
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
    T->NodeInfo.NumChild++;
    return trav->NextSibling;
  }
  return NULL;
}

Tree FindNode(Tree T)
{

}

Tree InsertNode(Tree T, struct TreeNode *Target)
{
}

#define DIRINFO 'D'
#define DIREND '.'

int SendTreeDataDriver(Tree T, char buffer[MaxBufferLength], int* lastindex)
{

  buffer[*lastindex] = DIRINFO;
  *lastindex += 1;

  memcpy(&buffer[*lastindex], &T->NodeInfo, sizeof(T->NodeInfo));
  *lastindex += sizeof(T->NodeInfo);

  struct TreeNode* trav = T->ChildDirectoryLL;

  while (trav != NULL)
  {
    SendTreeDataDriver(trav, buffer, lastindex);
    trav = trav->NextSibling;
  }

  buffer[*lastindex] = DIREND;
  *lastindex += 1;

  return 0;
}

char* SendTreeData(Tree T, char buffer[MaxBufferLength])
{
  int lastindex = 0;
  SendTreeDataDriver(T, buffer, &lastindex);
  return buffer;
}

int ReceiveTreeDataDriver(Tree T, char buffer[MaxBufferLength], int* lastindex)
{
  memcpy(&T->NodeInfo, &buffer[*lastindex], sizeof(T->NodeInfo));
  *lastindex += sizeof(T->NodeInfo);
  
  T->NodeInfo.NumChild = 0;
  while(buffer[*lastindex] == DIRINFO)
  {
    *lastindex += 1;
    struct TreeNode* Node = FindChild(T, "", 1, 1);

    ReceiveTreeDataDriver(Node, buffer, lastindex);
  }
  *lastindex += 1;
}

Tree ReceiveTreeData(char buffer[MaxBufferLength])
{
  Tree T = InitTree();
  int lastindex = 1;
  ReceiveTreeDataDriver(T, buffer, &lastindex);
  return T;
}

void PrintTree(Tree T, int indent)
{
  for (int i=0; i<indent; i++)
  {
    printf("\t");
  }
  printf("%s\n", T->NodeInfo.DirectoryName);
  struct TreeNode* trav = T->ChildDirectoryLL;
  while (trav != NULL)
  {
    PrintTree(trav, indent+1);
    trav = trav->NextSibling;
  }
}

void RandomTest()
{
  Tree T = InitTree();
  Tree T2 = FindChild(T, "Hello", 1, 0);
  Tree T3 = FindChild(T2, "Fuck YOU", 1, 0);
  Tree T4 = FindChild(T, "LOL", 1, 0);
  Tree T5 = FindChild(T3, "NOPLEASE", 1, 0);
  Tree T6 = FindChild(T4, "OAWKFEOWAJFAW", 1, 0);
  char Buffer[MaxBufferLength];
  SendTreeData(T, Buffer);
  Tree TRec = ReceiveTreeData(Buffer);
  printf("OG:\n");
  PrintTree(T, 0);
  printf("Test:\n");
  PrintTree(TRec, 0);
}

int main()
{
  RandomTest();
}