#include "fs_indexed.h"
#include <stdlib.h>
#include <string.h>

static int countFreeBlocks(const FileSystem *fs)
{
    int count = 0;
    FreeBlockNode *curr = fs->freeHead; // start from head of free block list
    while (curr != NULL)                // traverse until end of list
    {
        count++;
        curr = curr->next;
    }
    return count;
}

static int findFreeFIB(const FileSystem *fs) // find index of first free FIB entry, or -1 if none available
{
    for (int i = 0; i < MAX_FILES; i++) // loop through FIB status array
    {
        if (fs->fibStatus[i] == 0) // if entry is free (0), return its index
        {
            return i;
        }
    }
    return -1;
}

static int findFileByName(const FileSystem *fs, const char *filename) // find index of file with given name, or -1 if not found
{
    for (int i = 0; i < MAX_FILES; i++) // loop through FIB status array
    {
        if (fs->fibStatus[i] == 1 && strcmp(fs->files[i].fileName, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}

Block *allocateFreeBlock(FileSystem *fs) // remove and return a free block from the head of the free block list, or NULL if no free blocks available
{
    if (fs == NULL || fs->freeHead == NULL) // if filesystem is NULL or free block list is empty, return NULL
    {
        return NULL;
    }

    FreeBlockNode *temp = fs->freeHead; // store current head node in temp
    Block *freeBlock = temp->block;     // get the block pointer from temp node

    fs->freeHead = fs->freeHead->next; // move head pointer to next node in list
    if (fs->freeHead == NULL)          // if list is now empty after removing head, set tail to NULL as well
    {
        fs->freeTail = NULL;
    }

    free(temp);
    return freeBlock;
}

void returnFreeBlock(FileSystem *fs, Block *block) // add a block back to the tail of the free block list
{
    if (fs == NULL || block == NULL) // if filesystem or block is NULL, do nothing
    {
        return;
    }

    /* clear block data before returning it */
    memset(block->data, 0, BLOCK_SIZE);

    FreeBlockNode *newNode = (FreeBlockNode *)malloc(sizeof(FreeBlockNode)); // create a new node for the block being returned
    if (newNode == NULL)
    {
        printf("Error: unable to return block %d to free list.\n", block->blockNumber);
        return;
    }

    newNode->block = block;
    newNode->next = NULL;

    if (fs->freeTail == NULL) // if free block list is currently empty, set both head and tail to new node
    {
        fs->freeHead = newNode;
        fs->freeTail = newNode;
    }
    else // otherwise, add new node to end of list and update tail pointer
    {
        fs->freeTail->next = newNode;
        fs->freeTail = newNode;
    }
}

void printFreeBlocks(const FileSystem *fs) // utility function to print the block numbers of all free blocks in the filesystem
{
    if (fs == NULL)
    {
        return;
    }

    int count = countFreeBlocks(fs); // count the number of free blocks for display
    printf("Free Blocks (%d): ", count);

    FreeBlockNode *curr = fs->freeHead; // start from head of free block list
    while (curr != NULL)                // traverse until end of list
    {
        printf("[%d] -> ", curr->block->blockNumber);
        curr = curr->next;
    }
    printf("NULL\n");
}

void initFS(FileSystem *fs) // initialize the filesystem structure, including disk blocks, FIB table, and free block linked list
{
    if (fs == NULL)
    {
        return;
    }

    fs->freeHead = NULL; // initialize free block list pointers to NULL
    fs->freeTail = NULL; // initialize free block list pointers to NULL
    fs->fileCount = 0;   // initialize file count to 0

    /* initialize disk blocks */
    for (int i = 0; i < TOTAL_BLOCKS; i++)
    {
        memset(fs->disk[i].data, 0, BLOCK_SIZE); // clear block data
        fs->disk[i].blockNumber = i;             // set block number for each block
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
        FreeBlockNode *node = (FreeBlockNode *)malloc(sizeof(FreeBlockNode)); // create a new node for the current block
        if (node == NULL)
        {
            printf("Error: memory allocation failed while initializing filesystem.\n");
            return;
        }

        node->block = &fs->disk[i]; // set block pointer in node to current block
        node->next = NULL;          // initialize next pointer to NULL

        if (fs->freeTail == NULL) // if free block list is currently empty, set both head and tail to new node
        {
            fs->freeHead = node;
            fs->freeTail = node;
        }
        else // otherwise, add new node to end of list and update tail pointer
        {
            fs->freeTail->next = node;
            fs->freeTail = node;
        }
    }

    printf("Filesystem initialized with %d blocks of %d bytes each.\n",
           TOTAL_BLOCKS, BLOCK_SIZE);
}

int createFile(FileSystem *fs, const char *filename, int size) // create a new file with the given name and size, allocating necessary blocks and updating FIB
{
    if (fs == NULL || filename == NULL || size < 0) // validate input parameters
    {
        return 0;
    }

    if (fs->fileCount >= MAX_FILES) // check if maximum file count has been reached before attempting to create a new file
    {
        printf("Error: maximum number of files reached.\n");
        return 0;
    }

    if (findFileByName(fs, filename) != -1) // check if a file with the same name already exists before creating a new file
    {
        printf("Error: file '%s' already exists.\n", filename);
        return 0;
    }

    int fibIndex = findFreeFIB(fs); // find an available FIB entry for the new file, or -1 if none available
    if (fibIndex == -1)             // if no free FIB entry is found, return an error
    {
        printf("Error: no free FIB available.\n");
        return 0;
    }

    int dataBlocksNeeded = (size + BLOCK_SIZE - 1) / BLOCK_SIZE; // calculate the number of data blocks needed to store the file, rounding up to the nearest block
    int totalBlocksNeeded = dataBlocksNeeded + 1;                /* +1 for index block */

    if (countFreeBlocks(fs) < totalBlocksNeeded) // check if there are enough free blocks available to allocate for the new file (including index block), and return an error if not
    {
        printf("Error: not enough free blocks for file '%s'.\n", filename);
        return 0;
    }

    /* allocate index block first */
    Block *indexBlock = allocateFreeBlock(fs);
    if (indexBlock == NULL) // if allocation of index block fails, return an error
    {
        printf("Error: failed to allocate index block.\n");
        return 0;
    }

    /* store data block numbers inside index block->data */
    int *indexEntries = (int *)indexBlock->data;

    for (int i = 0; i < dataBlocksNeeded; i++) // loop to allocate each required data block for the file, and store their block numbers in the index block's data area
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

int deleteFile(FileSystem *fs, const char *filename) // delete the file with the given name, returning its blocks to the free block list and clearing its FIB entry
{
    if (fs == NULL || filename == NULL) // validate input parameters
    {
        return 0;
    }

    int fileIndex = findFileByName(fs, filename); // find the index of the file to be deleted by its name, or -1 if not found
    if (fileIndex == -1)                          // if file is not found, return an error
    {
        printf("Error: file '%s' not found.\n", filename);
        return 0;
    }

    FIB *file = &fs->files[fileIndex];                 // get a pointer to the FIB entry for the file being deleted
    int *indexEntries = (int *)file->indexBlock->data; // get a pointer to the array of data block numbers stored in the file's index block

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

void listFiles(const FileSystem *fs) // list all files currently in the filesystem by iterating through the FIB status array and printing details of each file that is currently in use (status = 1)
{
    if (fs == NULL)
    {
        return;
    }

    printf("\nRoot Directory Listing (%d files):\n", fs->fileCount);

    for (int i = 0; i < MAX_FILES; i++) // loop through FIB status array and print details of each file that is currently in use (status = 1)
    {
        if (fs->fibStatus[i] == 1) // if FIB entry is in use (1), print file details
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