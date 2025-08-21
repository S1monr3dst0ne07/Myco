#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

#include <stddef.h>
#include <stdint.h>

// Memory allocation tracking structure
typedef struct {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
    uint64_t allocation_id;
    int is_freed;
} MemoryAllocation;

// Memory usage statistics
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t free_count;
    size_t leak_count;
} MemoryStats;

// Initialize memory tracking system
void memory_tracker_init(void);

// Cleanup memory tracking system
void memory_tracker_cleanup(void);

// Memory allocation wrappers
void* tracked_malloc(size_t size, const char* file, int line, const char* function);
void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* function);
void* tracked_realloc(void* ptr, size_t size, const char* file, int line, const char* function);
void tracked_free(void* ptr, const char* file, int line, const char* function);
char* tracked_strdup(const char* str, const char* file, int line, const char* function);

// Memory tracking functions
void print_memory_usage(void);
void detect_memory_leaks(void);
void cleanup_all_memory(void);
void validate_memory_integrity(void);

// Get current memory statistics
MemoryStats get_memory_stats(void);

// Enable/disable memory tracking
void enable_memory_tracking(int enable);

#endif // MEMORY_TRACKER_H
