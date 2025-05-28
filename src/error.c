#include "myco.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void myco_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char message[MYCO_MAX_ERROR_LENGTH];
    vsnprintf(message, sizeof(message), format, args);
    
    va_end(args);
    
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

void *myco_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        myco_error("Out of memory");
    }
    return ptr;
}

void *myco_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        myco_error("Out of memory");
    }
    return new_ptr;
}

void myco_free(void *ptr) {
    free(ptr);
} 