#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "myco.h"
#include <stdbool.h>

typedef enum {
    VAL_NONE,
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOL,
    VAL_LIST,
    VAL_MAP,
    VAL_FUNCTION
} ValueType;

typedef struct Function {
    Node* params;
    Node* body;
    struct Environment* closure;
} Function;

typedef struct Value {
    ValueType type;
    union {
        double number;
        char* string;
        bool boolean;
        struct {
            struct Value* elements;
            size_t count;
        } list;
        struct {
            struct Value* keys;
            struct Value* values;
            size_t count;
        } map;
        Function function;
    } as;
} Value;

typedef struct Environment {
    struct Environment* parent;
    struct {
        const char* name;
        Value value;
    }* variables;
    size_t count;
    size_t capacity;
} Environment;

typedef struct {
    Environment* current_scope;
    Environment global_scope;
    const char* error;
    bool should_return;
    bool should_break;
    bool should_continue;
    Value return_value;
} Interpreter;

void init_interpreter(Interpreter* interpreter);
Value evaluate_node(Interpreter* interpreter, Node* node);
Value call_function(Interpreter* interpreter, Function func, Value* args, int arg_count);
void set_variable(Interpreter* interpreter, const char* name, Value value);
Value get_variable(Interpreter* interpreter, const char* name);

#endif // INTERPRETER_H 