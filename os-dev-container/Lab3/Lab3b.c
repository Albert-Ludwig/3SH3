#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define INT_SIZE 4
#define INT_COUNT 10
#define MEMORY_SIZE (INT_COUNT * INT_SIZE)

int main(void)
{
    int fd = open("numbers.bin", O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening numbers.bin");
        return 1;
    }

    // mmap file into memory (read-only)
    unsigned char *mmapfptr = (unsigned char *)mmap(
        NULL,
        MEMORY_SIZE,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0);

    if (mmapfptr == MAP_FAILED)
    {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    int intArray[INT_COUNT];

    // Copy each 4-byte chunk into intArray[i]
    for (int i = 0; i < INT_COUNT; i++)
    {
        memcpy(&intArray[i], mmapfptr + (i * INT_SIZE), INT_SIZE);
    }

    // Clean up mapping and fd
    if (munmap(mmapfptr, MEMORY_SIZE) != 0)
    {
        perror("munmap failed");
        // continue anyway
    }
    close(fd);

    long long sum = 0;
    for (int i = 0; i < INT_COUNT; i++)
    {
        sum += intArray[i];
    }

    printf("Sum of numbers = %lld\n", sum);
    return 0;
}