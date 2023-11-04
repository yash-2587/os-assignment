#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

// Struct for a node in the sub-chain
typedef struct SubChainNode {
    void *start;  // Start address of the segment
    size_t size;  // Size of the segment
    int type;     // 0 for HOLE, 1 for PROCESS
    struct SubChainNode *next;
    struct SubChainNode *prev;
} SubChainNode;

// Struct for a node in the main chain
typedef struct MainChainNode {
    void *start;  // Start address of the main chain
    size_t size;  // Size of the main chain
    SubChainNode *sub_chain;
    struct MainChainNode *next;
    struct MainChainNode *prev;
} MainChainNode;

MainChainNode *free_list_head = NULL;  // Head of the free list
void *mems_virtual_address = NULL;     // MeMS virtual address
void *mems_physical_address = NULL;    // MeMS physical address


void mems_init() {
    // Initialize MeMS system
    // Create the initial free list node with the whole available memory
    MainChainNode *main_chain_node = (MainChainNode *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (main_chain_node == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    main_chain_node->start = main_chain_node;
    main_chain_node->size = PAGE_SIZE;
    main_chain_node->sub_chain = NULL;
    main_chain_node->next = main_chain_node->prev = NULL;

    free_list_head = main_chain_node;
    mems_virtual_address = main_chain_node;
}

void mems_finish() {
    // Unmap allocated memory
    if (mems_virtual_address) {
        munmap(mems_virtual_address, PAGE_SIZE);
    }
}


void *mems_malloc(size_t size) {
    // Allocate memory using MeMS
    MainChainNode *main_chain_node = free_list_head;
    SubChainNode *sub_chain_node = NULL;
    size_t required_size = (size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

    while (main_chain_node != NULL) {
        SubChainNode *sub_node = main_chain_node->sub_chain;
        while (sub_node != NULL) {
            if (sub_node->type == 0 && sub_node->size >= required_size) {
                // Found a suitable HOLE segment in the sub-chain
                sub_chain_node = sub_node;
                break;
            }
            sub_node = sub_node->next;
        }
        if (sub_chain_node != NULL) {
            break;
        }
        main_chain_node = main_chain_node->next;
    }

    if (sub_chain_node != NULL) {
        // Reuse the HOLE segment
        if (sub_chain_node->size > required_size) {
            // Create a new HOLE segment with the remaining memory
            SubChainNode *new_sub_node = (SubChainNode *)malloc(sizeof(SubChainNode));
            new_sub_node->start = sub_chain_node->start + required_size;
            new_sub_node->size = sub_chain_node->size - required_size;
            new_sub_node->type = 0;
            new_sub_node->next = sub_chain_node->next;
            new_sub_node->prev = sub_chain_node;
            if (sub_chain_node->next != NULL) {
                sub_chain_node->next->prev = new_sub_node;
            }
            sub_chain_node->size = required_size;
            sub_chain_node->type = 1;
            sub_chain_node->next = new_sub_node;
        } else {
            // The entire HOLE segment is used
            sub_chain_node->type = 1;
        }
    } else {
        // No suitable segment found, allocate a new main chain node
        main_chain_node = (MainChainNode *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (main_chain_node == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        main_chain_node->start = main_chain_node;
        main_chain_node->size = PAGE_SIZE;
        main_chain_node->sub_chain = NULL;
        main_chain_node->next = free_list_head;
        main_chain_node->prev = NULL;
        free_list_head->prev = main_chain_node;
        free_list_head = main_chain_node;

        // Create a new PROCESS segment in the sub-chain
        sub_chain_node = (SubChainNode *)malloc(sizeof(SubChainNode));
        sub_chain_node->start = main_chain_node->start;
        sub_chain_node->size = required_size;
        sub_chain_node->type = 1;
        sub_chain_node->next = NULL;
        sub_chain_node->prev = NULL;
        main_chain_node->sub_chain = sub_chain_node;
    }

    return sub_chain_node->start;
}

void mems_print_stats() {
    size_t pages_utilized = 0;
    size_t unused_memory = 0;

    MeMSSegment* current = free_list;
    while (current) {
        unused_memory += current->size;
        pages_utilized++;
        current = current->next;
    }

    printf("--------- Printing Stats [mems_print_stats] --------\n");
    printf("---------------------MeMS SYSTEM STATS-----------------------\n\n");

    current = free_list;

    while (current) {
        if (current == free_list) {
            printf("MAIN[%lu:%lu] ->", (unsigned long)mems_virtual_address, (unsigned long)mems_virtual_address + current->size - 1);
        } else {
            printf("P[%lu:%lu]  <->", (unsigned long)current, (unsigned long)current + current->size - 1);
        }
        current = current->next;
    }

    printf("NULL\n");
    printf("Pages used: %zu\n", pages_utilized);
    printf("Space unused: %zu\n", unused_memory);
    printf("Main Chain Length: %zu\n", pages_utilized);

    current = free_list;
    printf("Sub-chain Length array: [");
    while (current) {
        printf("%zu", current->size);
        current = current->next;
        if (current) {
            printf(", ");
        }
    }
    printf("]\n");
}


void *mems_get(void *v_ptr) {
    // Get MeMS physical address corresponding to the provided MeMS virtual address
    return v_ptr;
    //return (void *)((char *)mems_physical_address + ((char *)v_ptr - (char *)mems_virtual_address));
}

void mems_free(void *v_ptr) {
    // Free memory pointed by the provided MeMS virtual address
    // Add it to the free list and update the sub-chain and main chain
    SubChainNode *sub_chain_node = NULL;
    MainChainNode *main_chain_node = free_list_head;
    while (main_chain_node != NULL) {
        SubChainNode *sub_node = main_chain_node->sub_chain;
        while (sub_node != NULL) {
            if (sub_node->start == v_ptr) {
                sub_chain_node = sub_node;
                break;
            }
            sub_node = sub_node->next;
        }
        if (sub_chain_node != NULL) {
            break;
        }
        main_chain_node = main_chain_node->next;
    }

    if (sub_chain_node != NULL) {
        // Mark the segment as a HOLE
        sub_chain_node->type = 0;

        // Merge adjacent HOLEs in the sub-chain
        if (sub_chain_node->prev != NULL && sub_chain_node->prev->type == 0) {
            sub_chain_node->prev->size += sub_chain_node->size;
            sub_chain_node->prev->next = sub_chain_node->next;
            if (sub_chain_node->next != NULL) {
                sub_chain_node->next->prev = sub_chain_node->prev;
            }
            free(sub_chain_node);
            sub_chain_node = sub_chain_node->prev;
        }
        if (sub_chain_node->next != NULL && sub_chain_node->next->type == 0) {
            sub_chain_node->size += sub_chain_node->next->size;
            sub_chain_node->next = sub_chain_node->next->next;
            if (sub_chain_node->next != NULL) {
                sub_chain_node->next->prev = sub_chain_node;
            }
            free(sub_chain_node->next);
        }

        // Merge adjacent HOLEs in the main chain
        main_chain_node = free_list_head;
        while (main_chain_node != NULL) {
            SubChainNode *sub_node = main_chain_node->sub_chain;
            while (sub_node != NULL) {
                if (sub_node->type == 0 && sub_node->prev != NULL && sub_node->prev->type == 0) {
                    sub_node->prev->size += sub_node->size;
                    sub_node->prev->next = sub_node->next;
                    if (sub_node->next != NULL) {
                        sub_node->next->prev = sub_node->prev;
                    }
                    free(sub_node);
                    sub_node = sub_node->prev;
                }
                sub_node = sub_node->next;
            }
            main_chain_node = main_chain_node->next;
        }
    }
}
