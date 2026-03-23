#include "fs_indexed.h"
#include <stdlib.h>
#include <string.h>

/* -------------------- helper functions -------------------- */

static int countFreeBlocks(const FileSystem *fs)
{
    int count = 0;
    FreeBlockNode *curr = fs->freeHead;
    while (curr != NULL)
    {
        count++;
        curr = curr->next;
    }
    return count;
}

static int findFreeFIB(const FileSystem *fs)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (fs->fibStatus[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

static int findFileByName(const FileSystem *fs, const char *filename)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (fs->fibStatus[i] == 1 && strcmp(fs->files[i].fileName, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}

/* -------------------- utility functions -------------------- */

Block *allocateFreeBlock(FileSystem *fs)
{
    if (fs == NULL || fs->freeHead == NULL)
    {
        return NULL;
    }

    FreeBlockNode *temp = fs->freeHead;
    Block *freeBlock = temp->block;

    fs->freeHead = fs->freeHead->next;
    if (fs->freeHead == NULL)
    {
        fs->freeTail = NULL;
    }

    free(temp);
    return freeBlock;
}

void returnFreeBlock(FileSystem *fs, Block *block)
{
    if (fs == NULL || block == NULL)
    {
        return;
    }

    /* clear block data before returning it */
    memset(block->data, 0, BLOCK_SIZE);

    FreeBlockNode *newNode = (FreeBlockNode *)malloc(sizeof(FreeBlockNode));
    if (newNode == NULL)
    {
        printf("Error: unable to return block %d to free list.\n", block->blockNumber);
        return;
    }

    newNode->block = block;
    newNode->next = NULL;

    if (fs->freeTail == NULL)
    {
        fs->freeHead = newNode;
        fs->freeTail = newNode;
    }
    else
    {
        fs->freeTail->next = newNode;
        fs->freeTail = newNode;
    }
}

void printFreeBlocks(const FileSystem *fs)
{
    if (fs == NULL)
    {
        return;
    }

    int count = countFreeBlocks(fs);
    printf("Free Blocks (%d): ", count);

    FreeBlockNode *curr = fs->freeHead;
    while (curr != NULL)
    {
        printf("[%d] -> ", curr->block->blockNumber);
        curr = curr->next;
    }
    printf("NULL\n");
}

/* -------------------- main required operations -------------------- */

void initFS(FileSystem *fs)
{
    if (fs == NULL)
    {
        return;
    }

    fs->freeHead = NULL;
    fs->freeTail = NULL;
    fs->fileCount = 0;

    /* initialize disk blocks */
    for (int i = 0; i < TOTAL_BLOCKS; i++)
    {
        memset(fs->disk[i].data, 0, BLOCK_SIZE);
        fs->disk[i].blockNumber = i;
    }

    /* initialize FIB table */
    for (int i = 0; i < MAX_FILES; i++)
    {
        fs->fibStatus[i] = 0;
        fs->files[i].fibID = i;
        fs->files[i].fileName[0] = '\0';
        fs->files[i].fileSize = 0;
        fs->files[i].blockCount = 0;
        fs->files[i].indexBlock = NULL;
        fs->files[i].inUse = 0;
    }

    /* build free block linked list with all blocks */
    for (int i = 0; i < TOTAL_BLOCKS; i++)
    {
        FreeBlockNode *node = (FreeBlockNode *)malloc(sizeof(FreeBlockNode));
        if (node == NULL)
        {
            printf("Error: memory allocation failed while initializing filesystem.\n");
            return;
        }

        node->block = &fs->disk[i];
        node->next = NULL;

        if (fs->freeTail == NULL)
        {
            fs->freeHead = node;
            fs->freeTail = node;
        }
        else
        {
            fs->freeTail->next = node;
            fs->freeTail = node;
        }
    }

    printf("Filesystem initialized with %d blocks of %d bytes each.\n",
           TOTAL_BLOCKS, BLOCK_SIZE);
}

int createFile(FileSystem *fs, const char *filename, int size)
{
    if (fs == NULL || filename == NULL || size < 0)
    {
        return 0;
    }

    if (fs->fileCount >= MAX_FILES)
    {
        printf("Error: maximum number of files reached.\n");
        return 0;
    }

    if (findFileByName(fs, filename) != -1)
    {
        printf("Error: file '%s' already exists.\n", filename);
        return 0;
    }

    int fibIndex = findFreeFIB(fs);
    if (fibIndex == -1)
    {
        printf("Error: no free FIB available.\n");
        return 0;
    }

    int dataBlocksNeeded = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int totalBlocksNeeded = dataBlocksNeeded + 1; /* +1 for index block */

    if (countFreeBlocks(fs) < totalBlocksNeeded)
    {
        printf("Error: not enough free blocks for file '%s'.\n", filename);
        return 0;
    }

    /* allocate index block first */
    Block *indexBlock = allocateFreeBlock(fs);
    if (indexBlock == NULL)
    {
        printf("Error: failed to allocate index block.\n");
        return 0;
    }

    /* store data block numbers inside index block->data */
    int *indexEntries = (int *)indexBlock->data;

    for (int i = 0; i < dataBlocksNeeded; i++)
    {
        Block *dataBlock = allocateFreeBlock(fs);
        if (dataBlock == NULL)
        {
            /* rollback already allocated blocks */
            for (int j = 0; j < i; j++)
            {
                int blkNum = indexEntries[j];
                returnFreeBlock(fs, &fs->disk[blkNum]);
            }
            returnFreeBlock(fs, indexBlock);
            printf("Error: failed while allocating data blocks.\n");
            return 0;
        }

        indexEntries[i] = dataBlock->blockNumber;
    }

    /* fill FIB */
    fs->fibStatus[fibIndex] = 1;
    fs->files[fibIndex].fibID = fibIndex;
    strncpy(fs->files[fibIndex].fileName, filename, MAX_FILENAME - 1);
    fs->files[fibIndex].fileName[MAX_FILENAME - 1] = '\0';
    fs->files[fibIndex].fileSize = size;
    fs->files[fibIndex].blockCount = dataBlocksNeeded;
    fs->files[fibIndex].indexBlock = indexBlock;
    fs->files[fibIndex].inUse = 1;

    fs->fileCount++;

    printf("File '%s' created with %d data blocks + 1 index block.\n",
           filename, dataBlocksNeeded);

    return 1;
}

int deleteFile(FileSystem *fs, const char *filename)
{
    if (fs == NULL || filename == NULL)
    {
        return 0;
    }

    int fileIndex = findFileByName(fs, filename);
    if (fileIndex == -1)
    {
        printf("Error: file '%s' not found.\n", filename);
        return 0;
    }

    FIB *file = &fs->files[fileIndex];
    int *indexEntries = (int *)file->indexBlock->data;

    /* return all data blocks to tail */
    for (int i = 0; i < file->blockCount; i++)
    {
        int blkNum = indexEntries[i];
        returnFreeBlock(fs, &fs->disk[blkNum]);
    }

    /* return index block last */
    returnFreeBlock(fs, file->indexBlock);

    /* clear FIB */
    fs->fibStatus[fileIndex] = 0;
    file->fileName[0] = '\0';
    file->fileSize = 0;
    file->blockCount = 0;
    file->indexBlock = NULL;
    file->inUse = 0;

    fs->fileCount--;

    printf("File '%s' deleted.\n", filename);
    return 1;
}

void listFiles(const FileSystem *fs)
{
    if (fs == NULL)
    {
        return;
    }

    printf("\nRoot Directory Listing (%d files):\n", fs->fileCount);

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (fs->fibStatus[i] == 1)
        {
            printf("  %-10s | %6d bytes | %2d data blocks | FIBID=%d\n",
                   fs->files[i].fileName,
                   fs->files[i].fileSize,
                   fs->files[i].blockCount,
                   fs->files[i].fibID);
        }
    }

    printf("\n");
}