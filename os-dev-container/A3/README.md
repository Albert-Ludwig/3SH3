# 3SH3 A3 README

## Group Name

Group - 2

## Student number and name

Johnson Ji 400499564 \
Hongliang Qi 400493278

## Assignment contribution description:

In the A3, I, the student Johnson Ji, develop the logic algorithm of the FIFO paging in/out strategy and the TLB working logic, and doing the test for the output.

## Explaination for this assignment:

This assignment is a simple virtual memory simulator written in C. The goal is to translate logical addresses into physical addresses by simulating how an MMU works with a page table, a TLB, and physical memory. The program reads logical addresses from addresses.txt, separates each address into a page number and an offset, and then tries to find the corresponding frame number. The required translation process is: check the TLB first, then the page table, and if the page is not in memory, generate a page fault.

In my program, the page table is implemented as an array, and the TLB is implemented using a small structure that stores the page number, frame number, and valid bit. The TLB uses FIFO replacement, so when it becomes full, the oldest entry is removed first. The code includes three main TLB functions: search_TLB, TLB_Add, and TLB_Update, which follow the assignment requirements.

When a page fault happens, the program loads the required 256-byte page from BACKING_STORE.bin into physical memory using mmap() and memcpy(). Since physical memory is smaller than logical memory, page replacement is needed after memory becomes full. FIFO page replacement is used, meaning the oldest page in memory is replaced first. After replacement, the page table and TLB are updated so that future translations use the correct mapping.

For each logical address, the program prints the logical address, the physical address, and the signed byte value stored at that physical address. At the end, it also prints the total number of addresses, the total number of page faults, and the total number of TLB hits, which are part of the required output for the assignment.
