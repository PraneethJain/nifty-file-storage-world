#ifndef __TREE_H
#define __TREE_H

#include "defs.h"
u8 plus_one(u8 x);

struct Information
{
  char DirectoryName[MAX_STR_LEN];
  u8 NumChild;
  bool IsFile;
  /*
  Extra Information
  */
};

struct TreeNode
{
  struct Information NodeInfo;
  struct TreeNode *NextSibling;
  struct TreeNode *PrevSibling;
  struct TreeNode *ChildDirectoryLL; // LL - > Linked List
  struct TreeNode *Parent;
};

typedef struct TreeNode *Tree;

Tree InitTree();

void AddAccessibleDir(char *DirPath, Tree Parent);

void PrintTree(Tree T, u32 indent);

#endif