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
    // Print MeMS system statistics
    int pages_used = 0;
    size_t space_unused = 0;
    MainChainNode *main_chain_node = free_list_head;
    int main_chain_length = 0;
    int sub_chain_lengths[100];  // Assuming a maximum of 100 sub-chains
    for (int i = 0; i < 100; i++) {
        sub_chain_lengths[i] = 0;
    }

    printf("----- MEMS SYSTEM STATS -----\n");

    while (main_chain_node != NULL) {
        main_chain_length++;
        printf("MAIN[%p:%p]->", main_chain_node->start, main_chain_node->start + main_chain_node->size - 1);
        SubChainNode *sub_chain_node = main_chain_node->sub_chain;
        while (sub_chain_node != NULL) {
            sub_chain_lengths[main_chain_length - 1]++;
            if (sub_chain_node->type == 0) {
                space_unused += sub_chain_node->size;
                printf(" H[%p:%p] <->", sub_chain_node->start, sub_chain_node->start + sub_chain_node->size - 1);
            } else {
                pages_used++;
                printf(" P[%p:%p] <->", sub_chain_node->start, sub_chain_node->start + sub_chain_node->size - 1);
            }
            sub_chain_node = sub_chain_node->next;
        }
        printf(" NULL\n");
        main_chain_node = main_chain_node->next;
    }

    printf("Pages used: %d\n", pages_used);
    printf("Space unused: %zu\n", space_unused);
    printf("Main Chain Length: %d\n", main_chain_length);
    printf("Sub-chain Length array: [");
    for (int i = 0; i < main_chain_length; i++) {
        printf("%d", sub_chain_lengths[i]);
        if (i < main_chain_length - 1) {
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
