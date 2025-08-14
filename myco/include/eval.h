#ifndef EVAL_H
#define EVAL_H

#include "parser.h"

// Array data structure
typedef struct {
    long long* elements;     // Dynamic array of integers
    char** str_elements;     // Dynamic array of strings
    int capacity;            // Current allocated capacity
    int size;                // Current number of elements
    int is_string_array;     // Flag for string vs integer arrays
} MycoArray;

// Array management function prototypes
MycoArray* create_array(int initial_capacity, int is_string_array);
void destroy_array(MycoArray* array);
int array_push(MycoArray* array, void* element);
void* array_get(MycoArray* array, int index);
int array_set(MycoArray* array, int index, void* element);
int array_size(MycoArray* array);
int array_capacity(MycoArray* array);
void cleanup_array_env();
MycoArray* get_array_value(const char* name);
void set_array_value(const char* name, MycoArray* array);

// Function prototypes
void eval_evaluate(ASTNode* ast);
void eval_set_base_dir(const char* dir);
void eval_clear_module_asts();
void eval_clear_function_asts();

#endif // EVAL_H 