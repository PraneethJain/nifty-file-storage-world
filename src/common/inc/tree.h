#ifndef __TREE_H
#define __TREE_H

#include "defs.h"
u8 plus_one(u8 x);

struct Information
{
  char DirectoryName[MAX_DIRNAME_LEN];
  u8 NumChild;
  bool IsFile;
  bool Access;
  u32 ss_id;
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
int SendTreeData(Tree T, char *buffer);
Tree ReceiveTreeData(char *buffer);
void MergeTree(Tree T1, Tree T2, u32 ss_id);

void RemoveServerPath(Tree T, u32 ss_id);
i32 GetPathSSID(Tree T, const char *path);
char *get_parent(const char *path);

void AddFile(Tree T, const char *path);
void AddFolder(Tree T, const char *path);
void DeleteFile(Tree T, const char *path);
void DeleteFolder(Tree T, const char *path);

void PrintTree(Tree T, u32 indent);

#endif