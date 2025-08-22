#ifndef EVAL_H
#define EVAL_H

#include "parser.h"

// Implicit function system for operator overloading
typedef struct {
    char* operator;           // The operator symbol (e.g., "+", "==", "<")
    char* function_name;      // The function to call (e.g., "add", "equals", "less_than")
    int precedence;           // Operator precedence level
    int associativity;        // LEFT_ASSOC or RIGHT_ASSOC
    int supports_types[4];    // Array of supported type combinations
} OperatorMapping;

// Type combinations for implicit functions
#define TYPE_COMBINATION_NUMERIC 0    // number + number
#define TYPE_COMBINATION_STRING  1    // string + string
#define TYPE_COMBINATION_ARRAY   2    // array + array
#define TYPE_COMBINATION_OBJECT  3    // object + object

// Associativity constants
#define LEFT_ASSOC  0
#define RIGHT_ASSOC 1

// Array data structure
typedef struct {
    long long* elements;     // Dynamic array of integers
    char** str_elements;     // Dynamic array of strings
    int capacity;            // Current allocated capacity
    int size;                // Current number of elements
    int is_string_array;     // Flag for string vs integer arrays
} MycoArray;

// Property type enumeration
typedef enum {
    PROP_TYPE_NUMBER,   // Numeric value (long long cast to void*)
    PROP_TYPE_STRING,   // String value (char*)
    PROP_TYPE_OBJECT    // Nested object (MycoObject*)
} PropertyType;

// Object data structure for key-value pairs
typedef struct MycoObject {
    char** property_names;      // Dynamic array of property names
    void** property_values;     // Dynamic array of property values
    PropertyType* property_types; // Dynamic array of property types
    int property_count;         // Current number of properties
    int capacity;               // Current allocated capacity
    int is_method;              // Flag for future method support
} MycoObject;

// Set data structure for unique collections
typedef struct MycoSet {
    void** elements;             // Dynamic array of unique elements
    int element_count;           // Current number of elements
    int capacity;                // Current allocated capacity
    int is_string_set;           // Flag for string vs integer sets
} MycoSet;

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
int object_set_property_typed(MycoObject* obj, const char* name, void* value, PropertyType type);
int object_set_property(MycoObject* obj, const char* name, void* value);
PropertyType object_get_property_type(MycoObject* obj, const char* name);
void* object_get_property(MycoObject* obj, const char* name);
int object_has_property(MycoObject* obj, const char* name);
void cleanup_object_env();
MycoObject* get_object_value(const char* name);
void set_object_value(const char* name, MycoObject* obj);

// Set management function prototypes
MycoSet* create_set(int initial_capacity, int is_string_set);
void destroy_set(MycoSet* set);
int set_add(MycoSet* set, void* element);
int set_has(MycoSet* set, void* element);
int set_remove(MycoSet* set, void* element);
int set_size(MycoSet* set);
void cleanup_set_env();
MycoSet* get_set_value(const char* name);
void set_set_value(const char* name, MycoSet* set);

// Function prototypes
void eval_evaluate(ASTNode* ast);
void eval_set_base_dir(const char* dir);
void eval_clear_module_asts();
void eval_clear_function_asts();
void cleanup_all_environments(void);

// Implicit function system prototypes
void init_implicit_functions(void);
char* get_implicit_function(const char* operator, int left_type, int right_type);
long long call_implicit_function(const char* function_name, ASTNode* children, int child_count);
int get_type_combination(int left_type, int right_type);
void cleanup_implicit_functions(void);

#endif // EVAL_H 