#ifndef __TREE_H
#define __TREE_H

#include "defs.h"
u8 plus_one(u8 x);

struct Information
{
  char DirectoryName[MAX_NAME_LEN];
  u8 NumChild;
  bool IsFile;
  bool Access;
  u32 ss_id;
  char UUID[MAX_STR_LEN];
  pthread_rwlock_t rwlock;
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
void InitDirectory(Tree Parent);
void RemoveInaccessiblePath(Tree Parent, const char *DirPath);
int SendTreeData(Tree T, char *buffer);
Tree ReceiveTreeData(char *buffer);
void MergeTree(Tree T1, Tree T2, u32 ss_id, char *UUID);

void RemoveServerPath(Tree T, u32 ss_id);
i32 GetPathSSID(Tree T, const char *path);
char *get_parent(const char *path);
Tree GetTreeFromPath(Tree T, const char *path);
i8 Is_File(Tree T, const char *path);

void AddFile(Tree T, const char *path, i32 port_ss_nm, char *UUID);
void AddFolder(Tree T, const char *path, i32 port_ss_nm, char *UUID);
void DeleteFile(Tree T, const char *path);
void DeleteFolder(Tree T, const char *path);
i8 ancestor(Tree T, const char *from_path, const char *to_path);

void AcquireReaderLock(Tree T, const char *path);
void AcquireWriterLock(Tree T, const char *path);
void ReleaseLock(Tree T, const char *path);

void PrintTree(Tree T, u32 indent);

#endif
