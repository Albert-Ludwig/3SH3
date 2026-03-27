#include "fs_indexed.h"

int main(void)
{
    FileSystem fs;

    initFS(&fs);

    createFile(&fs, "alpha.txt", 3072);
    createFile(&fs, "beta.txt", 5120);
    listFiles(&fs);
    printFreeBlocks(&fs);

    deleteFile(&fs, "alpha.txt");
    listFiles(&fs);
    printFreeBlocks(&fs);

    createFile(&fs, "gamma.txt", 4096);
    createFile(&fs, "delta.txt", 8192);
    listFiles(&fs);
    printFreeBlocks(&fs);

    deleteFile(&fs, "beta.txt");
    deleteFile(&fs, "gamma.txt");
    deleteFile(&fs, "delta.txt");
    listFiles(&fs);
    printFreeBlocks(&fs);

    return 0;
}