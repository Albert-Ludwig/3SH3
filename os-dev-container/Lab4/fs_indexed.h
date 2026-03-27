#ifndef FS_INDEXED_H
#define FS_INDEXED_H

#include <stdio.h>

#define BLOCK_SIZE 1024
#define TOTAL_BLOCKS 64
#define MAX_FILES 10
#define MAX_FILENAME 100

typedef struct Block
{
    unsigned char data[BLOCK_SIZE];
    int blockNumber;
} Block;

typedef struct FreeBlockNode
{
    Block *block;
    struct FreeBlockNode *next;
} FreeBlockNode;

typedef struct FIB
{
    int fibID;
    char fileName[MAX_FILENAME];
    int fileSize;      // in bytes
    int blockCount;    // number of data blocks
    Block *indexBlock; // points to index block
    int inUse;         // 0 = free, 1 = occupied
} FIB;

typedef struct Volume_control_block
{
    FreeBlockNode *freeHead;
    FreeBlockNode *freeTail;

    Block disk[TOTAL_BLOCKS];

    int fibStatus[MAX_FILES]; // 0 = free, 1 = used
    FIB files[MAX_FILES];

    int fileCount;
} Volume_control_block;
typedef struct FileSystem
{
    Volume_control_block vcb;
};

/* required operations */
void initFS(FileSystem *fs);
int createFile(FileSystem *fs, const char *filename, int size);
int deleteFile(FileSystem *fs, const char *filename);
void listFiles(const FileSystem *fs);

/* utility functions */
Block *allocateFreeBlock(FileSystem *fs);
void returnFreeBlock(FileSystem *fs, Block *block);
void printFreeBlocks(const FileSystem *fs);

#endif