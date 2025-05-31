#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define HEAP_SIZE 1836311903  
#define MAX_FIB_COUNT 50  

// Global Fibonacci table and count
size_t fib_numbers[MAX_FIB_COUNT];
int fib_count = 0;

// Heap management
void *heap_start = NULL;
size_t simulated_heap_size = 0;

typedef struct BlockHeader {
    size_t size;          // Total size of block
    size_t req_size;      // Original requested size
    int fib_index;        // Index in Fibonacci table
    int is_free;          // 1 if free, 0 if allocated
    struct BlockHeader *next_free; 
    struct BlockHeader *prev_free; 
} BlockHeader;

BlockHeader *free_list_head = NULL;

// Function prototypes 
void init_fib_table();
void init_heap();
void insert_free_block(BlockHeader *block);
void remove_free_block(BlockHeader *block);
void print_free_list();
void *simulate_malloc(size_t size);
void simulate_free(void *ptr);
void try_merge(BlockHeader *block);
void validate_free_list();
size_t align_size(size_t size);

// Initialize Fibonacci sequence up to HEAP_SIZE
void init_fib_table() {
    fib_numbers[0] = 1;
    fib_numbers[1] = 2;
    fib_count = 2;
    
    while (fib_count < MAX_FIB_COUNT) {
        size_t next = fib_numbers[fib_count - 1] + fib_numbers[fib_count - 2];
        if (next > HEAP_SIZE) break;
        fib_numbers[fib_count++] = next;
    }
    
    printf("Fibonacci sequence initialized (%d terms):\n", fib_count);
    for (int i = 0; i < fib_count; i++) {
        printf("F%d=%zu ", i+1, fib_numbers[i]);
        if ((i+1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

// Initialize heap with largest Fibonacci block <= HEAP_SIZE
void init_heap() {
    heap_start = malloc(HEAP_SIZE);
    if (!heap_start) {
        perror("Failed to allocate simulated heap");
        exit(EXIT_FAILURE);
    }
    
    // Find largest Fibonacci number <= HEAP_SIZE
    int largest_index = fib_count - 1;
    while (largest_index >= 0 && fib_numbers[largest_index] > HEAP_SIZE) {
        largest_index--;
    }
    
    if (largest_index < 0) {
        fprintf(stderr, "No suitable Fibonacci block found for heap size\n");
        exit(EXIT_FAILURE);
    }
    
    simulated_heap_size = fib_numbers[largest_index];
    printf("Initializing heap with size %zu (F%d)\n", 
           simulated_heap_size, largest_index+1);
    
    // Create initial free block
    BlockHeader *initial_block = (BlockHeader *)heap_start;
    initial_block->size = simulated_heap_size;
    initial_block->req_size = 0;
    initial_block->fib_index = largest_index;
    initial_block->is_free = 1;
    initial_block->next_free = initial_block->prev_free = NULL;
    free_list_head = initial_block;
}

// Insert block into free list in address order
void insert_free_block(BlockHeader *block) {
    assert(block != NULL);
    block->is_free = 1;
    
    if (free_list_head == NULL) {
        free_list_head = block;
        block->next_free = block->prev_free = NULL;
        return;
    }
    
    BlockHeader *current = free_list_head;
    BlockHeader *prev = NULL;
    
    // Find insertion point (sorted by address)
    while (current && current < block) {
        prev = current;
        current = current->next_free;
    }
    
    // Insert before current
    block->next_free = current;
    block->prev_free = prev;
    
    if (prev) {
        prev->next_free = block;
    } else {
        free_list_head = block;
    }
    
    if (current) {
        current->prev_free = block;
    }
}

// Remove block from free list
void remove_free_block(BlockHeader *block) {
    assert(block != NULL);
    
    if (block->prev_free) {
        block->prev_free->next_free = block->next_free;
    } else {
        free_list_head = block->next_free;
    }
    
    if (block->next_free) {
        block->next_free->prev_free = block->prev_free;
    }
    
    block->next_free = block->prev_free = NULL;
    block->is_free = 0;
}

// Print current state of free list
void print_free_list() {
    printf("\nCurrent free list:\n");
    BlockHeader *current = free_list_head;
    int count = 0;
    
    while (current) {
        printf("  [%d] Addr: %p, Size: %zu (F%d)\n", 
               ++count, (void*)current, current->size, current->fib_index+1);
        current = current->next_free;
    }
    
    if (count == 0) {
        printf("  (empty)\n");
    }
    printf("Total free memory: %zu bytes\n\n", simulated_heap_size - allocated_memory);
}

// Round up size to nearest Fibonacci number
size_t align_size(size_t size) {
    size_t total_size = size + sizeof(BlockHeader);
    
    for (int i = 0; i < fib_count; i++) {
        if (fib_numbers[i] >= total_size) {
            return fib_numbers[i];
        }
    }
    
    return 0; // Indicates size too large
}

// Track allocated memory
size_t allocated_memory = 0;

// Allocate memory using Fibonacci buddy system
void *simulate_malloc(size_t size) {
    if (size == 0) return NULL;
    
    size_t aligned_size = align_size(size);
    if (aligned_size == 0) {
        printf("Allocation failed: requested size %zu too large\n", size);
        return NULL;
    }
    
    int target_index = -1;
    for (int i = 0; i < fib_count; i++) {
        if (fib_numbers[i] == aligned_size) {
            target_index = i;
            break;
        }
    }
    
    // Search for exact fit first
    BlockHeader *block = NULL;
    BlockHeader *current = free_list_head;
    
    while (current) {
        if (current->fib_index == target_index) {
            block = current;
            break;
        }
        current = current->next_free;
    }
    
    // If no exact fit, find smallest possible larger block
    if (!block) {
        current = free_list_head;
        while (current) {
            if (current->fib_index > target_index) {
                if (!block || current->fib_index < block->fib_index) {
                    block = current;
                }
            }
            current = current->next_free;
        }
    }
    
    // If found a suitable block
    if (block) {
        // Split block until we get desired size
        while (block->fib_index > target_index) {
            remove_free_block(block);
            
            int left_index = block->fib_index - 1;
            int right_index = block->fib_index - 2;
            
            // Create left buddy (Fn-1)
            BlockHeader *left = block;
            left->size = fib_numbers[left_index];
            left->fib_index = left_index;
            left->is_free = 1;
            
            // Create right buddy (Fn-2)
            BlockHeader *right = (BlockHeader*)((char*)block + fib_numbers[left_index]);
            right->size = fib_numbers[right_index];
            right->fib_index = right_index;
            right->is_free = 1;
            
            // Insert both into free list
            insert_free_block(left);
            insert_free_block(right);
            
            // Continue with left buddy
            block = left;
        }
        
        // Found exact size block
        remove_free_block(block);
        block->req_size = size;
        allocated_memory += block->size;
        
        printf("Allocated %zu bytes (actual %zu) at %p (F%d)\n",
               size, block->size, (void*)(block + 1), block->fib_index + 1);
        
        return (void*)(block + 1);
    }
    
    printf("Allocation failed for %zu bytes (no suitable block)\n", size);
    return NULL;
}

// Free allocated memory and merge buddies
void simulate_free(void *ptr) {
    if (!ptr) return;
    
    BlockHeader *block = ((BlockHeader*)ptr) - 1;
    assert(!block->is_free);
    
    allocated_memory -= block->size;
    printf("Freeing %zu bytes at %p (F%d)\n", 
           block->size, ptr, block->fib_index + 1);
    
    insert_free_block(block);
    try_merge(block);
}

// Merge adjacent free blocks when possible
void try_merge(BlockHeader *block) {
    int merged;
    
    do {
        merged = 0;
        
        // Check right buddy
        BlockHeader *right = (BlockHeader*)((char*)block + block->size);
        if ((char*)right < (char*)heap_start + simulated_heap_size && 
            right->is_free && 
            right->fib_index == block->fib_index - 1) {
            
            // Remove both from free list
            remove_free_block(block);
            remove_free_block(right);
            
            // Create merged block
            block->size += right->size;
            block->fib_index++;
            block->req_size = 0;
            
            // Reinsert merged block
            insert_free_block(block);
            merged = 1;
            continue;
        }
        
        // Check left buddy
        if (block != heap_start) {
            // Find previous block in free list that might be left buddy
            BlockHeader *current = free_list_head;
            BlockHeader *left = NULL;
            
            while (current) {
                if ((char*)current + current->size == (char*)block) {
                    left = current;
                    break;
                }
                current = current->next_free;
            }
            
            if (left && left->is_free && left->fib_index == block->fib_index - 1) {
                // Remove both from free list
                remove_free_block(left);
                remove_free_block(block);
                
                // Create merged block
                left->size += block->size;
                left->fib_index++;
                left->req_size = 0;
                
                // Reinsert merged block
                insert_free_block(left);
                block = left;
                merged = 1;
            }
        }
    } while (merged);
}

// Validate free list integrity
void validate_free_list() {
    printf("Validating free list...\n");
    
    BlockHeader *current = free_list_head;
    BlockHeader *prev = NULL;
    int count = 0;
    
    while (current) {
        count++;
        assert(current->is_free);
        
        if (prev) {
            assert(prev < current);
            assert(prev->next_free == current);
            assert(current->prev_free == prev);
        } else {
            assert(current == free_list_head);
            assert(current->prev_free == NULL);
        }
        
        prev = current;
        current = current->next_free;
    }
    
    printf("Free list validation passed (%d blocks)\n", count);
}

int main() {
    init_fib_table();
    init_heap();
    
    void *ptrs[5];
    size_t sizes[5];
    
    // Interactive allocation
    for (int i = 0; i < 5; i++) {
        printf("Enter size for allocation %d: ", i+1);
        scanf("%zu", &sizes[i]);
        ptrs[i] = simulate_malloc(sizes[i]);
        
        if (!ptrs[i]) {
            printf("Allocation failed. Exiting.\n");
            return 1;
        }
        
        print_free_list();
        validate_free_list();
    }
    
    // Free all allocations
    printf("\nFreeing all allocations...\n");
    for (int i = 0; i < 5; i++) {
        if (ptrs[i]) {
            simulate_free(ptrs[i]);
            print_free_list();
            validate_free_list();
        }
    }
    
    free(heap_start);
    return 0;
}
