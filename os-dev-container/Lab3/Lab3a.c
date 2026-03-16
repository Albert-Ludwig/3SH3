#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BUFFER_SIZE 256
#define OFFSET_BITS 12
#define OFFSET_MASK ((1U << OFFSET_BITS) - 1U) // 0xFFF low 12 bits

int main(void)
{
    // Page table given by the lab
    int page_table[8] = {6, 4, 3, 7, 0, 1, 2, 5};

    FILE *fptr = fopen("labaddr.txt", "r");
    if (fptr == NULL)
    {
        perror("Error opening labaddr.txt");
        return 1;
    }

    char buff[BUFFER_SIZE];

    while (fgets(buff, sizeof(buff), fptr) != NULL)
    {
        // Convert line to integer logical address
        // strtoul is safer than atoi and handles whitespace/newline well
        char *endptr = NULL;
        unsigned long logical_ul = strtoul(buff, &endptr, 10);

        // Basic validation: if no conversion happened
        if (endptr == buff)
        {
            // Skip invalid line
            continue;
        }

        uint32_t logical = (uint32_t)logical_ul;

        uint32_t page = logical >> OFFSET_BITS;
        uint32_t offset = logical & OFFSET_MASK;

        // The lab says we only use 8 pages (0..7). Still, guard it.
        if (page >= 8)
        {
            printf("Virtual addr is %u: Page# = %u & Offset = %u. Physical addr = INVALID (page out of range).\n",
                   logical, page, offset);
            continue;
        }

        uint32_t frame = (uint32_t)page_table[page];
        uint32_t physical = (frame << OFFSET_BITS) | offset;

        printf("Virtual addr is %u: Page# = %u & Offset = %u. Physical addr = %u.\n",
               logical, page, offset, physical);
    }

    fclose(fptr);
    return 0;
}