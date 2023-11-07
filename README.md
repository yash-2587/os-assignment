# os-assignment
This system provides dynamic memory allocation and management for virtual addresses, simulating memory allocation within a predefined memory range.

Memory Structure
simulated_memory: This is a character array representing a simulated memory space with a maximum size of MAX_MEM bytes (10,240 bytes). This array is used to simulate memory allocation and deallocation.

PAGE_SIZE: Defines the page size for memory allocation, set to 4,096 bytes (4 KB). This is the granularity at which memory is managed.

virtual_address_counter: A counter used to keep track of the current virtual address for memory allocation.

space_unused and space_unused_main_chain: These variables are used to keep track of the amount of unused memory in the main chain and are initially set to the page size.

Data Structures
The code defines two main data structures:

SubChainNode:
Represents a sub-chain of memory within a page.
Contains information about the start address, size, type (0 for unused, 1 for used), and pointers to the next and previous nodes in the sub-chain.
MainChainNode:
Represents a main chain of pages.
Contains information about the start address, size, a pointer to the associated sub-chain, and pointers to the next and previous nodes in the main chain.
Main Functions

mems_compact():

This function iterates through the main chain and sub-chains to compact memory by moving used memory blocks to reduce fragmentation. It combines adjacent free blocks within a page.
**ONE OF THE EDGE CASES TO REDUCE FRAGMENTATION**
perform_memory_management():

Calculates the percentage of unused memory in the main chain and triggers compaction if it exceeds a specified threshold (50%).
mems_init():

Initializes the memory management system. It sets up the initial main chain with a single page and initializes the virtual address counter.
mems_finish():

Deallocates memory and releases resources when the MEMS system is no longer needed.
create_main_chain_node():

Dynamically allocates and initializes a new main chain node (a page).
create_sub_chain_node():

Dynamically allocates and initializes a new sub-chain node.
mems_malloc(size_t size):

Allocates memory of the specified size. It searches for an appropriate location in the main chain to allocate memory and returns a pointer to the allocated memory.
mems_print_stats():

Prints detailed statistics about the memory usage, including the state of the main chain, sub-chains, pages used, and space unused.
mems_get(void* v_ptr):

Given a virtual pointer, this function converts it to the corresponding physical address within the simulated memory space.
mems_free(void* v_ptr):

Frees the memory associated with a virtual pointer, marking it as unused and potentially merging adjacent free memory blocks within a page.
How It Works
The mems_init() function sets up the initial page and initializes the main chain with it.
Memory is allocated with mems_malloc(), which searches for available space in the main chain or creates new pages as needed.
Memory is deallocated with mems_free(), marking the appropriate sub-chain node as unused and potentially merging adjacent free memory blocks.
The mems_compact() function is used to reduce fragmentation by moving used memory blocks and merging free blocks within a page.
mems_print_stats() displays statistics about the memory usage, including the structure of the main chain and sub-chains.
This system is designed to efficiently manage memory within the defined simulated memory space while allowing for dynamic allocation and deallocation of memory blocks.


**WE HAVE TRIED TO COVER EDGE CASES LIKE MEMORY COMPACTION MEMORY LEAK ETC.** 
AND ALL THE NECESSARY DETAILS ARE PROVIDED IN THIS README.


CODED BY-
KARANJEET SINGH (2022235)
YASH GARG(2022587)
