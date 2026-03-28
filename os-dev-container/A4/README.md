# 3SH3 A3 README

## Group Name

Group - 2

## Student number and name

Johnson Ji 400499564 \
Hongliang Qi 400493278

## Assignment contribution description:

I, the student Johnson Ji, write the header file and the logic of the function in fs_indexed.c, the student Hongliang Qi write the main.c and test the file.

## Explain for this assignment:

This assignment implements a simplified indexed file allocation simulator in C. The goal is to model how a basic file system manages files, free blocks, and deletion using an index block approach.

The file system is represented by fixed-size disk blocks, a free-block linked list, and a File Information Block (FIB) table. Each created file occupies one index block plus several data blocks, where the index block stores the block numbers of all data blocks belonging to that file.

Main operations included in this project:

1. Initialize file system (`initFS`): reset disk blocks, initialize FIB entries, and build the free-block queue.
2. Create file (`createFile`): check file constraints, allocate index/data blocks, and update file metadata.
3. Delete file (`deleteFile`): release all data blocks and index block back to the free list, then clear FIB metadata.
4. List files (`listFiles`): print root-directory style file information including name, size, and number of blocks.
5. Display free blocks (`printFreeBlocks`): visualize current free-space state in linked-list order.

The `main.c` program demonstrates a full lifecycle of file-system behavior by creating files, listing directory contents, deleting files, and showing changes in free-block allocation after each operation.
