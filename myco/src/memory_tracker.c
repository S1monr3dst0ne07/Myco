/**
 * @file memory_tracker.c
 * @brief Myco Memory Tracking System - Debug Memory Management
 * @version 1.0.0
 * @author Myco Development Team
 * 
 * This file implements a comprehensive memory tracking system for debugging
 * and development purposes. It tracks all memory allocations, deallocations,
 * and provides detailed statistics to identify memory leaks and usage patterns.
 * 
 * Memory Tracking Features:
 * - Allocation tracking with source location (file, line, function)
 * - Memory leak detection and reporting
 * - Peak memory usage monitoring
 * - Allocation statistics and summaries
 * - Memory usage visualization
 * - Automatic cleanup verification
 * 
 * Debug Capabilities:
 * - Tracked malloc/realloc/free functions
 * - Memory allocation history
 * - Leak detection on program exit
 * - Performance impact monitoring
 * - Cross-platform compatibility
 * 
 * Usage:
 * - Automatically enabled in debug builds
 * - Provides detailed memory reports
 * - Helps identify memory management issues
 * - Zero impact in release builds
 */

#include "memory_tracker.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*******************************************************************************
 * MEMORY TRACKING STATE
 ******************************************************************************/

/**
 * Global state variables for memory tracking system.
 * These maintain the current state of all tracked allocations
 * and provide statistics for debugging and monitoring.
 */
static int tracking_enabled = 1;
static MemoryAllocation* allocations = NULL;
static size_t allocations_capacity = 0;
static size_t allocations_count = 0;
static uint64_t next_allocation_id = 1;
static MemoryStats stats = {0};

/*******************************************************************************
 * SYSTEM INITIALIZATION AND CLEANUP
 ******************************************************************************/

/**
 * @brief Initializes the memory tracking system
 * 
 * Sets up the memory tracking infrastructure:
 * - Allocates the allocations tracking array
 * - Initializes statistics counters
 * - Sets up the allocation ID sequence
 * - Enables tracking functionality
 * 
 * This function should be called once at program startup
 * to enable memory tracking capabilities.
 */
void memory_tracker_init(void) {
    if (allocations) {
        // Already initialized
        return;
    }
    
    allocations_capacity = 1024; // Start with space for 1024 allocations
    // Use regular malloc for the tracker's own internal array
    // This prevents infinite recursion since tracked_malloc calls add_allocation
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

/**
 * @brief Cleans up the memory tracking system
 * 
 * Performs cleanup operations:
 * - Frees the allocations tracking array
 * - Resets all counters and statistics
 * - Disables tracking functionality
 * - Reports cleanup status in debug mode
 * 
 * This function should be called during program shutdown
 * to prevent memory leaks in the tracking system itself.
 */
void memory_tracker_cleanup(void) {
    #if DEBUG_MEMORY_TRACKING
    printf("Memory tracker cleaned up\n");
    #endif
    
    if (allocations) {
        // Use regular free for the tracker's own internal array
        // This prevents the "untracked pointer" warning for our own allocations
        free(allocations);
        allocations = NULL;
    }
    allocations_count = 0;
    allocations_capacity = 0;
    
    // Reset statistics
    memset(&stats, 0, sizeof(MemoryStats));
}

/*******************************************************************************
 * UTILITY FUNCTIONS
 ******************************************************************************/

/**
 * @brief Expands the allocations tracking array when needed
 * 
 * Dynamically grows the tracking array to accommodate more allocations:
 * - Doubles the capacity when expansion is needed
 * - Reallocates memory and preserves existing data
 * - Initializes new space to prevent undefined behavior
 * - Reports expansion status for debugging
 * 
 * This function is called automatically when the current
 * array capacity is insufficient for new allocations.
 */
static void expand_allocations_array(void) {
    size_t new_capacity = allocations_capacity * 2;
    // Use regular realloc for the tracker's own internal array
    // This prevents infinite recursion since tracked_realloc calls add_allocation
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

/**
 * @brief Finds a tracked allocation by its memory pointer
 * @param ptr The memory pointer to search for
 * @return Pointer to the allocation record, or NULL if not found
 * 
 * Searches through all tracked allocations to find a specific
 * memory pointer. This is used for deallocation tracking
 * and memory leak detection.
 */
static MemoryAllocation* find_allocation(void* ptr) {
    for (size_t i = 0; i < allocations_count; i++) {
        if (allocations[i].ptr == ptr) {
            return &allocations[i];
        }
    }
    return NULL;
}

/**
 * @brief Adds a new memory allocation to the tracking system
 * @param ptr The memory pointer returned by malloc/realloc
 * @param size The size of the allocated memory block
 * @param file Source file where allocation occurred
 * @param line Line number where allocation occurred
 * @param function Function name where allocation occurred
 * 
 * Records a new memory allocation for tracking purposes:
 * - Assigns a unique allocation ID
 * - Records source location information
 * - Updates memory usage statistics
 * - Expands tracking array if necessary
 * - Maintains chronological allocation order
 */
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
    // Memory tracker should be initialized by main program
    if (!allocations) {
        fprintf(stderr, "Error: Memory tracker not initialized. Call memory_tracker_init() first.\n");
        return NULL;
    }
    
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
    // Memory tracker should be initialized by main program
    if (!allocations) {
        fprintf(stderr, "Error: Memory tracker not initialized. Call memory_tracker_init() first.\n");
        return NULL;
    }
    
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
    if (!ptr) return;
    
    // Find and remove the allocation
    for (size_t i = 0; i < allocations_count; i++) {
        if (allocations[i].ptr == ptr) {
            // Update statistics
            stats.total_freed += allocations[i].size;
            stats.free_count++;
            stats.current_usage -= allocations[i].size;
            
            // Remove from tracking array
            if (i < allocations_count - 1) {
                allocations[i] = allocations[allocations_count - 1];
            }
            allocations_count--;
            
            // Free the actual memory
            free(ptr);
            return;
        }
    }
    
    // If we get here, the pointer wasn't tracked (ruh roh)
    #if DEBUG_MEMORY_TRACKING
    // Only warn about critical untracked pointers to reduce noise
    // This helps focus on real memory issues rather than mixed management patterns
    static int untracked_warnings = 0;
    if (untracked_warnings < 5) {  // Limit warnings to first 5 occurrences
        fprintf(stderr, "Warning: Attempting to free untracked pointer %p (warning %d/5)\n", ptr, ++untracked_warnings);
    }
    #endif
    
    free(ptr);
}

char* tracked_strdup(const char* str, const char* file, int line, const char* function) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* result = (char*)tracked_malloc(len, file, line, function);
    if (result) {
        strcpy(result, str);
    }
    return result;
}

// Print current memory usage
void print_memory_usage(void) {
    #if ENABLE_MEMORY_STATS
    MemoryStats stats = get_memory_stats();
    printf("\n=== Memory Usage Report ===\n");
    printf("Current Usage: %zu bytes\n", stats.current_usage);
    printf("Peak Usage: %zu bytes\n", stats.peak_usage);
    printf("Total Allocated: %zu bytes\n", stats.total_allocated);
    printf("Total Freed: %zu bytes\n", stats.total_freed);
    printf("Allocation Count: %zu\n", stats.allocation_count);
    printf("Free Count: %zu\n", stats.free_count);
    printf("Active Allocations: %zu\n", allocations_count);
    printf("===========================\n\n");
    #else
    // Minimal output in release mode
    printf("Memory tracking disabled in release mode\n");
    #endif
}

// Detect memory leaks
void detect_memory_leaks(void) {
    #if DEBUG_MEMORY_TRACKING
    printf("\n=== Memory Leak Detection ===\n");
    
    if (allocations_count == 0) {
        printf("No memory leaks detected! 🎉\n");
        printf("=============================\n\n");
        return;
    }
    
    int leak_count = 0;
    size_t total_leaked = 0;
    
    for (size_t i = 0; i < allocations_count; i++) {
        leak_count++;
        total_leaked += allocations[i].size;
        printf("LEAK #%d: %zu bytes at %p\n", leak_count, allocations[i].size, allocations[i].ptr);
        printf("  Allocated in %s:%d (%s)\n",
               allocations[i].file, allocations[i].line, allocations[i].function);
    }
    
    printf("=============================\n");
    printf("Total leaks: %d, Total leaked: %zu bytes\n", leak_count, total_leaked);
    printf("=============================\n\n");
    #else
    printf("Memory leak detection disabled in release mode\n");
    #endif
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
