#include "memory_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Memory tracking state
static int tracking_enabled = 1;
static MemoryAllocation* allocations = NULL;
static size_t allocations_capacity = 0;
static size_t allocations_count = 0;
static uint64_t next_allocation_id = 1;
static MemoryStats stats = {0};

// Initialize memory tracking system
void memory_tracker_init(void) {
    if (allocations) {
        // Already initialized
        return;
    }
    
    allocations_capacity = 1024; // Start with space for 1024 allocations
    allocations = malloc(allocations_capacity * sizeof(MemoryAllocation));
    if (!allocations) {
        fprintf(stderr, "Warning: Failed to initialize memory tracker\n");
        tracking_enabled = 0;
        return;
    }
    
    memset(allocations, 0, allocations_capacity * sizeof(MemoryAllocation));
    memset(&stats, 0, sizeof(MemoryStats));
    next_allocation_id = 1;
    
    printf("Memory tracker initialized with capacity for %zu allocations\n", allocations_capacity);
}

// Cleanup memory tracking system
void memory_tracker_cleanup(void) {
    if (!allocations) {
        return;
    }
    
    // Print final memory statistics
    print_memory_usage();
    
    // Detect any remaining memory leaks
    detect_memory_leaks();
    
    // Free the tracking array itself
    free(allocations);
    allocations = NULL;
    allocations_capacity = 0;
    allocations_count = 0;
    
    printf("Memory tracker cleaned up\n");
}

// Expand allocations array if needed
static void expand_allocations_array(void) {
    size_t new_capacity = allocations_capacity * 2;
    MemoryAllocation* new_allocations = realloc(allocations, new_capacity * sizeof(MemoryAllocation));
    
    if (!new_allocations) {
        fprintf(stderr, "Warning: Failed to expand memory tracker array\n");
        return;
    }
    
    // Clear new space
    memset(new_allocations + allocations_capacity, 0, 
           (new_capacity - allocations_capacity) * sizeof(MemoryAllocation));
    
    allocations = new_allocations;
    allocations_capacity = new_capacity;
    
    printf("Memory tracker expanded to capacity %zu\n", new_capacity);
}

// Find allocation by pointer
static MemoryAllocation* find_allocation(void* ptr) {
    for (size_t i = 0; i < allocations_count; i++) {
        if (allocations[i].ptr == ptr) {
            return &allocations[i];
        }
    }
    return NULL;
}

// Add new allocation to tracking
static void add_allocation(void* ptr, size_t size, const char* file, int line, const char* function) {
    if (!tracking_enabled || !allocations) {
        return;
    }
    
    // Expand array if needed
    if (allocations_count >= allocations_capacity) {
        expand_allocations_array();
    }
    
    // Add new allocation
    MemoryAllocation* alloc = &allocations[allocations_count++];
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->file = file;
    alloc->line = line;
    alloc->function = function;
    alloc->allocation_id = next_allocation_id++;
    alloc->is_freed = 0;
    
    // Update statistics
    stats.total_allocated += size;
    stats.current_usage += size;
    stats.allocation_count++;
    
    if (stats.current_usage > stats.peak_usage) {
        stats.peak_usage = stats.current_usage;
    }
}

// Mark allocation as freed
static void mark_allocation_freed(void* ptr, size_t size) {
    if (!tracking_enabled || !allocations) {
        return;
    }
    
    MemoryAllocation* alloc = find_allocation(ptr);
    if (alloc && !alloc->is_freed) {
        alloc->is_freed = 1;
        stats.total_freed += size;
        stats.current_usage -= size;
        stats.free_count++;
    }
}

// Memory allocation wrappers
void* tracked_malloc(size_t size, const char* file, int line, const char* function) {
    void* ptr = malloc(size);
    if (ptr) {
        add_allocation(ptr, size, file, line, function);
    }
    return ptr;
}

void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* function) {
    void* ptr = calloc(nmemb, size);
    if (ptr) {
        add_allocation(ptr, nmemb * size, file, line, function);
    }
    return ptr;
}

void* tracked_realloc(void* ptr, size_t size, const char* file, int line, const char* function) {
    if (ptr) {
        // Find old allocation to get its size
        MemoryAllocation* old_alloc = find_allocation(ptr);
        size_t old_size = old_alloc ? old_alloc->size : 0;
        
        void* new_ptr = realloc(ptr, size);
        if (new_ptr) {
            if (old_alloc) {
                // Update existing allocation
                old_alloc->ptr = new_ptr;
                old_alloc->size = size;
                old_alloc->file = file;
                old_alloc->line = line;
                old_alloc->function = function;
                
                // Update statistics
                stats.total_allocated += size - old_size;
                stats.current_usage += size - old_size;
            } else {
                // New allocation
                add_allocation(new_ptr, size, file, line, function);
            }
        }
        return new_ptr;
    } else {
        // New allocation
        return tracked_malloc(size, file, line, function);
    }
}

void tracked_free(void* ptr, const char* file, int line, const char* function) {
    if (!ptr) {
        return;
    }
    
    MemoryAllocation* alloc = find_allocation(ptr);
    if (alloc) {
        size_t size = alloc->size;
        mark_allocation_freed(ptr, size);
    }
    
    free(ptr);
}

// Print current memory usage
void print_memory_usage(void) {
    printf("\n=== Memory Usage Report ===\n");
    printf("Current Usage: %zu bytes\n", stats.current_usage);
    printf("Peak Usage: %zu bytes\n", stats.peak_usage);
    printf("Total Allocated: %zu bytes\n", stats.total_allocated);
    printf("Total Freed: %zu bytes\n", stats.total_freed);
    printf("Allocation Count: %zu\n", stats.allocation_count);
    printf("Free Count: %zu\n", stats.free_count);
    printf("Active Allocations: %zu\n", allocations_count);
    printf("===========================\n\n");
}

// Detect memory leaks
void detect_memory_leaks(void) {
    if (!allocations) {
        return;
    }
    
    int leak_count = 0;
    size_t total_leaked = 0;
    
    printf("\n=== Memory Leak Detection ===\n");
    
    for (size_t i = 0; i < allocations_count; i++) {
        if (!allocations[i].is_freed) {
            leak_count++;
            total_leaked += allocations[i].size;
            
            printf("LEAK #%d: %zu bytes at %p\n", leak_count, allocations[i].size, allocations[i].ptr);
            printf("  Allocated in %s:%d (%s)\n", 
                   allocations[i].file ? allocations[i].file : "unknown",
                   allocations[i].line,
                   allocations[i].function ? allocations[i].function : "unknown");
        }
    }
    
    if (leak_count == 0) {
        printf("No memory leaks detected! 🎉\n");
    } else {
        printf("Total leaks: %d, Total leaked: %zu bytes\n", leak_count, total_leaked);
    }
    
    printf("=============================\n\n");
    
    stats.leak_count = leak_count;
}

// Get current memory statistics
MemoryStats get_memory_stats(void) {
    return stats;
}

// Enable/disable memory tracking
void enable_memory_tracking(int enable) {
    tracking_enabled = enable;
    printf("Memory tracking %s\n", enable ? "enabled" : "disabled");
}
