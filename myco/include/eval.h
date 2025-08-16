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

// Object data structure for key-value pairs
typedef struct MycoObject {
    char** property_names;   // Dynamic array of property names
    void** property_values;  // Dynamic array of property values
    int property_count;      // Current number of properties
    int capacity;            // Current allocated capacity
    int is_method;           // Flag for future method support
} MycoObject;

// Array management function prototypes
MycoArray* create_array(int initial_capacity, int is_string_array);
void destroy_array(MycoArray* array);
int array_push(MycoArray* array, void* element);
void* array_get(MycoArray* array, int index);
const char* array_get_string(MycoArray* array, int index);
int array_set(MycoArray* array, int index, void* element);
int array_size(MycoArray* array);
int array_capacity(MycoArray* array);
void cleanup_array_env();
MycoArray* get_array_value(const char* name);
void set_array_value(const char* name, MycoArray* array);

// Object management function prototypes
MycoObject* create_object(int initial_capacity);
void destroy_object(MycoObject* obj);
int object_set_property(MycoObject* obj, const char* name, void* value);
void* object_get_property(MycoObject* obj, const char* name);
int object_has_property(MycoObject* obj, const char* name);
void cleanup_object_env();
MycoObject* get_object_value(const char* name);
void set_object_value(const char* name, MycoObject* obj);

// Function prototypes
void eval_evaluate(ASTNode* ast);
void eval_set_base_dir(const char* dir);
void eval_clear_module_asts();
void eval_clear_function_asts();

#endif // EVAL_H 