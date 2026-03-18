#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define LOGICAL_MEMORY_SIZE 65536
#define PHYSICAL_MEMORY_SIZE 32768
#define FRAME_COUNT (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)

// Each TLB entry stores one page -> frame mapping
typedef struct
{
    int page;
    int frame;
    int valid;
} TLBEntry;

// page_table[page] = frame number, or -1 if page is not in physical memory
int page_table[PAGE_TABLE_SIZE];

// simulated physical memory, each frame is 256 bytes
signed char physical_memory[PHYSICAL_MEMORY_SIZE];

// frame_to_page[frame] tells which page is currently stored in that frame
int frame_to_page[FRAME_COUNT];

// TLB array
TLBEntry tlb[TLB_SIZE];

// next_free_frame is used before memory becomes full
// next_replace_frame is used for FIFO page replacement after memory is full
int next_free_frame = 0;
int next_replace_frame = 0;

// current number of valid entries in the TLB
int tlb_count = 0;

// counters for final statistics
int page_faults = 0;
int tlb_hits = 0;
int total_addresses = 0;

// Search the TLB first
// If page is found, return its frame number
// Otherwise return -1
int search_TLB(int page_number)
{
    for (int i = 0; i < tlb_count; i++)
    {
        if (tlb[i].valid && tlb[i].page == page_number) // TLB hit
        {
            return tlb[i].frame;
        }
    }
    return -1;
}

// Add a new page-frame mapping into the TLB using FIFO
void TLB_Add(int page_number, int frame_number)
{
    // If TLB still has space, just append the new entry
    if (tlb_count < TLB_SIZE)
    {
        tlb[tlb_count].page = page_number;
        tlb[tlb_count].frame = frame_number;
        tlb[tlb_count].valid = 1;
        tlb_count++;
    }
    else
    {
        // TLB is full, so remove the oldest entry by shifting left
        for (int i = 0; i < TLB_SIZE - 1; i++)
        {
            tlb[i] = tlb[i + 1];
        }

        // Put the newest entry at the end
        tlb[TLB_SIZE - 1].page = page_number;
        tlb[TLB_SIZE - 1].frame = frame_number;
        tlb[TLB_SIZE - 1].valid = 1;
    }
}

// This function is used when a page in physical memory gets replaced
// If the old page is still in the TLB, replace that TLB entry directly
// If old page is not in the TLB, just add the new page normally
void TLB_Update(int old_page, int new_page, int new_frame)
{
    for (int i = 0; i < tlb_count; i++)
    {
        if (tlb[i].valid && tlb[i].page == old_page) // Found old_page in TLB, replace it with new_page
        {
            tlb[i].page = new_page;
            tlb[i].frame = new_frame;
            tlb[i].valid = 1;
            return;
        }
    }

    // old_page was not in TLB, so just insert the new page
    TLB_Add(new_page, new_frame);
}

// Handle a page fault by loading the page from backing store into physical memory
// Return the frame number where the page is loaded
int handle_page_fault(int page_number, signed char *backing_store)
{
    int frame_number;

    page_faults++;

    // If physical memory still has unused frames, use the next free one
    if (next_free_frame < FRAME_COUNT)
    {
        frame_number = next_free_frame;
        next_free_frame++;
    }
    else
    {
        // Physical memory is full, so use FIFO replacement
        frame_number = next_replace_frame;
        int old_page = frame_to_page[frame_number];

        // The old page is no longer in physical memory
        page_table[old_page] = -1;

        // Also update the TLB if needed
        TLB_Update(old_page, page_number, frame_number);

        // Move FIFO pointer to the next frame
        next_replace_frame = (next_replace_frame + 1) % FRAME_COUNT;
    }

    // Copy one whole page (256 bytes) from backing store into the chosen frame
    memcpy(&physical_memory[frame_number * PAGE_SIZE],
           &backing_store[page_number * PAGE_SIZE],
           PAGE_SIZE);

    // Update page table and reverse mapping
    page_table[page_number] = frame_number;
    frame_to_page[frame_number] = page_number;

    return frame_number;
}

int main(void)
{
    FILE *address_file;
    int backing_store_fd;
    signed char *backing_store;

    // Initialize page table to -1, meaning no page is loaded at the start
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        page_table[i] = -1;
    }

    // Initialize frame-to-page map
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        frame_to_page[i] = -1;
    }

    // Initialize TLB entries as invalid
    for (int i = 0; i < TLB_SIZE; i++)
    {
        tlb[i].valid = 0;
        tlb[i].page = -1;
        tlb[i].frame = -1;
    }

    // Open the backing store file
    backing_store_fd = open("BACKING_STORE.bin", O_RDONLY);
    if (backing_store_fd == -1)
    {
        perror("Error opening BACKING_STORE.bin");
        return 1;
    }

    // Map the backing store into memory for easier access
    backing_store = mmap(0, LOGICAL_MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_store_fd, 0);
    if (backing_store == MAP_FAILED)
    {
        perror("Error mapping BACKING_STORE.bin");
        close(backing_store_fd);
        return 1;
    }

    // Open the logical addresses input file
    address_file = fopen("addresses.txt", "r");
    if (address_file == NULL)
    {
        perror("Error opening addresses.txt");
        munmap(backing_store, LOGICAL_MEMORY_SIZE);
        close(backing_store_fd);
        return 1;
    }

    int logical_address;

    // Process each logical address one by one
    while (fscanf(address_file, "%d", &logical_address) == 1)
    {
        total_addresses++;

        // Extract page number and offset from the 16-bit logical address
        int page_number = (logical_address >> 8) & 0xFF;
        int offset = logical_address & 0xFF;

        // Step 1: try TLB first
        int frame_number = search_TLB(page_number);

        if (frame_number != -1)
        {
            // TLB hit, so we already know the frame number
            tlb_hits++;
        }
        else
        {
            // TLB miss, so check the page table
            frame_number = page_table[page_number];

            if (frame_number == -1)
            {
                // Page fault: page is not in physical memory
                int had_free_frame = (next_free_frame < FRAME_COUNT);

                frame_number = handle_page_fault(page_number, backing_store);

                // If a free frame was used, TLB has not been updated yet
                if (had_free_frame)
                {
                    TLB_Add(page_number, frame_number);
                }
            }
            else
            {
                // TLB miss but page table hit, so add this mapping into TLB
                TLB_Add(page_number, frame_number);
            }
        }

        // Build the physical address using frame number and offset
        int physical_address = frame_number * PAGE_SIZE + offset;

        // Read the signed byte value stored at that physical address
        signed char value = physical_memory[physical_address];

        // Print result in the required format
        printf("Virtual address: %d Physical address = %d Value=%d\n",
               logical_address, physical_address, value);
    }

    // Print summary statistics at the end
    printf("Total addresses = %d\n", total_addresses);
    printf("Page_faults = %d\n", page_faults);
    printf("TLB Hits = %d\n", tlb_hits);

    fclose(address_file);
    munmap(backing_store, LOGICAL_MEMORY_SIZE);
    close(backing_store_fd);

    return 0;
}