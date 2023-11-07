#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#define MAX_MEM 10240
char simulated_memory[MAX_MEM];
#define PAGE_SIZE 4096
static size_t virtual_address_counter = 1000; 
size_t space_unused = 0;
size_t space_unused_main_chain = PAGE_SIZE;

typedef struct SubChainNode {
    void *start; 
    size_t size;
    int type;    
    struct SubChainNode *next;
    struct SubChainNode *prev;
} SubChainNode;
typedef struct MainChainNode {
    void *start; 
    size_t size;
    SubChainNode *sub_chain;
    struct MainChainNode *next;
    struct MainChainNode *prev;
} MainChainNode;

MainChainNode *free_list_head = NULL; 
void *mems_virtual_address = NULL;    
void *mems_physical_address = NULL;

void mems_compact() {
    MainChainNode *current = free_list_head;

    while (current != NULL) {
        SubChainNode *sub_node = current->sub_chain;

        while (sub_node != NULL) {
            if (sub_node->type == 1) {
                if (sub_node->start != current->start) {
                    void *new_start = current->start;
                    SubChainNode *prev_node = sub_node->prev;

                    if (prev_node != NULL && prev_node->type == 0) {
                        new_start = prev_node->start;
                        prev_node->size += sub_node->size;
                        prev_node->next = sub_node->next;

                        if (sub_node->next != NULL) {
                            sub_node->next->prev = prev_node;
                        }

                        free(sub_node);
                        sub_node = prev_node;
                    } else {
                        sub_node->start = new_start;
                    }
                    if (sub_node->start != current->start) {
                        current->start = new_start;
                    }
                }
            }

            sub_node = sub_node->next;
        }

        current = current->next;
    }
}

void perform_memory_management() {

    double unused_memory_percentage = 0.5;
    size_t total_memory = 0;
    size_t unused_memory = 0;

    MainChainNode *main_chain_node = free_list_head;
    while (main_chain_node != NULL) {
        total_memory += main_chain_node->size;
        
        SubChainNode *sub_chain_node = main_chain_node->sub_chain;
        while (sub_chain_node != NULL) {
            if (sub_chain_node->type == 0) {
                unused_memory += sub_chain_node->size;
            }
            sub_chain_node = sub_chain_node->next;
        }
        
        main_chain_node = main_chain_node->next;
    }
    if ((double)unused_memory / total_memory >= unused_memory_percentage) {
        mems_compact();
    }
}

void mems_init() {
    MainChainNode *main_chain_node = (MainChainNode *)mmap((void *)1000, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (main_chain_node == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    main_chain_node->start = (void *)1000;
    main_chain_node->size = PAGE_SIZE;
    main_chain_node->sub_chain = NULL;
    main_chain_node->next = main_chain_node->prev = NULL;

    free_list_head = main_chain_node;
    mems_virtual_address = main_chain_node;
    virtual_address_counter = 1000;
}


void mems_finish() {
    if (mems_virtual_address) {
        if (munmap(mems_virtual_address, PAGE_SIZE) == -1) {
            perror("munmap");
            exit(1);
        }
    }
}
static size_t last_virtual_address = 1000;
MainChainNode *create_main_chain_node() {
    MainChainNode *node = (MainChainNode *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (node == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    node->start = (void *)last_virtual_address;
    node->size = PAGE_SIZE;
    node->sub_chain = NULL;
    node->next = NULL;
    node->prev = NULL;
    return node;
}
SubChainNode *create_sub_chain_node(size_t size) {
    SubChainNode *node = (SubChainNode *)malloc(sizeof(SubChainNode));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->size = size;
    node->type = 1;
    node->next = NULL;
    node->prev = NULL;
    return node;
}
void *mems_malloc(size_t size) {
    MainChainNode *main_chain_node = free_list_head;
    if (main_chain_node == NULL) {
        main_chain_node = create_main_chain_node();
        main_chain_node->start = (void *)1000;
        free_list_head = main_chain_node;
        space_unused_main_chain -= PAGE_SIZE;
        space_unused = space_unused_main_chain;
    } else {
        while (main_chain_node->next != NULL) {
            main_chain_node = main_chain_node->next;
        }
    }
    if (last_virtual_address % PAGE_SIZE + size > PAGE_SIZE) {
        size_t space_to_next_page = PAGE_SIZE - (last_virtual_address % PAGE_SIZE);

        if (space_to_next_page < size) {
            MainChainNode *new_node = create_main_chain_node();
            main_chain_node->next = new_node;
            new_node->prev = main_chain_node;
            main_chain_node = new_node;
            space_unused_main_chain += PAGE_SIZE;
            last_virtual_address += space_to_next_page;
        }
    }
    SubChainNode *new_sub_node = create_sub_chain_node(size);
    new_sub_node->start = (void *)last_virtual_address;
    if (main_chain_node->sub_chain == NULL) {
        main_chain_node->sub_chain = new_sub_node;
    } else {
        SubChainNode *last_sub_node = main_chain_node->sub_chain;
        while (last_sub_node->next != NULL) {
            last_sub_node = last_sub_node->next;
        }
        last_sub_node->next = new_sub_node;
        new_sub_node->prev = last_sub_node;
    }
    last_virtual_address += size;

    space_unused_main_chain -= size;
    space_unused = space_unused_main_chain;
    return new_sub_node->start;
}

void mems_print_stats() {
    // Print MeMS system statistics
    size_t pages_used = 0;
    size_t space_unused = 0;
    MainChainNode *main_chain_node = free_list_head;
    int main_chain_length = 0;
    int sub_chain_lengths[100];
    for (int i = 0; i < 100; i++) {
        sub_chain_lengths[i] = 0;
    }

    printf("----- MEMS SYSTEM STATS -----\n");

    while (main_chain_node != NULL) {
        main_chain_length++;
        size_t main_start = (size_t)main_chain_node->start;
        size_t main_end = main_start + main_chain_node->size - 1;
        printf("MAIN[%zu:%zu]->", main_start, main_end);
        SubChainNode *sub_chain_node = main_chain_node->sub_chain;
        while (sub_chain_node != NULL) {
            sub_chain_lengths[main_chain_length - 1]++;
            size_t sub_start = (size_t)sub_chain_node->start;
            size_t sub_end = sub_start + sub_chain_node->size - 1;
            if (sub_chain_node->type == 0) {
                space_unused += sub_chain_node->size;
                printf(" H[%zu:%zu] <->", sub_start, sub_end);
            } else {
                pages_used++;
                printf(" P[%zu:%zu] <->", sub_start, sub_end);
            }
            sub_chain_node = sub_chain_node->next;
        }
        printf(" NULL\n");
        main_chain_node = main_chain_node->next;
    }

    printf("Pages used: %d\n",main_chain_length );
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
    size_t virtual_address = (size_t)v_ptr;
    size_t offset = virtual_address % MAX_MEM; 
    return (void *)&simulated_memory[offset];
}

void mems_free(void *v_ptr) {
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
        sub_chain_node->type = 0;
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
            SubChainNode *next_node = sub_chain_node->next->next; 
            free(sub_chain_node->next);
            sub_chain_node->next = next_node;
            if (next_node != NULL) {
                next_node->prev = sub_chain_node;
            }
        }
        if (main_chain_node->sub_chain == sub_chain_node && sub_chain_node->prev == NULL) {
            virtual_address_counter = (size_t)sub_chain_node->start;
        }
    }
}
