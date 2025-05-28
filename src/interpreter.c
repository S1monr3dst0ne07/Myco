#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "myco.h"
#include "interpreter.h"

// Runtime state
static Environment* current_env;
static Value return_value;
static bool has_return;

// Add a global Interpreter* pointer for the current interpreter
static Interpreter* current_interpreter = NULL;

// Add error handling state
static bool has_error = false;
static char* error_message = NULL;

// Forward declarations
static Value evaluate(Node* node);
static Environment* new_environment(Environment* parent);
static void define_variable(Environment* env, const char* name, Value value);
static void free_value(Value value);
static const char* node_type_str(NodeType type);

// Environment management
static Environment* new_environment(Environment* parent) {
    Environment* env = malloc(sizeof(Environment));
    env->parent = parent;
    env->count = 0;
    env->capacity = 8;
    env->variables = malloc(sizeof(*env->variables) * env->capacity);
    return env;
}

// Update get_variable to use Interpreter*
Value get_variable(Interpreter* interpreter, const char* name) {
    Environment* env = interpreter->current_scope;
    while (env != NULL) {
        for (size_t i = 0; i < env->count; i++) {
            if (strcmp(env->variables[i].name, name) == 0) {
                return env->variables[i].value;
            }
        }
        env = env->parent;
    }
    Value none = {VAL_NONE};
    return none;
}

// Update define_variable to use Interpreter* if needed
void set_variable(Interpreter* interpreter, const char* name, Value value) {
    Environment* env = interpreter->current_scope;
    // Check if variable already exists
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            env->variables[i].value = value;
            return;
        }
    }
    // Grow if needed
    if (env->count >= env->capacity) {
        env->capacity = env->capacity ? env->capacity * 2 : 8;
        env->variables = realloc(env->variables, sizeof(*env->variables) * env->capacity);
    }
    env->variables[env->count].name = strdup(name);
    env->variables[env->count].value = value;
    env->count++;
}

// Value management
static void free_value(Value value) {
    switch (value.type) {
        case VAL_STRING:
            free(value.as.string);
            break;
        case VAL_LIST:
            for (size_t i = 0; i < value.as.list.count; i++) {
                free_value(value.as.list.elements[i]);
            }
            free(value.as.list.elements);
            break;
        case VAL_MAP:
            for (size_t i = 0; i < value.as.map.count; i++) {
                free_value(value.as.map.keys[i]);
                free_value(value.as.map.values[i]);
            }
            free(value.as.map.keys);
            free(value.as.map.values);
            break;
        case VAL_FUNCTION:
            // Don't free the AST nodes, they're managed by the parser
            break;
        default:
            break;
    }
}

// Evaluation
static Value make_none() {
    Value value = {VAL_NONE, {0}};
    return value;
}

static Value make_number(double num) {
    Value value = {VAL_NUMBER, {.number = num}};
    return value;
}

static Value make_bool(bool b) {
    Value value = {VAL_BOOL, {.boolean = b}};
    return value;
}

static Value make_string(const char* str) {
    Value value = {VAL_STRING, {.string = strdup(str)}};
    return value;
}

static Value make_list() {
    Value value = {VAL_LIST, {.list = {malloc(sizeof(Value) * 8), 0, 8}}};
    return value;
}

static void list_add(Value* list, Value value) {
    if (list->as.list.count >= list->as.list.capacity) {
        list->as.list.capacity *= 2;
        list->as.list.elements = realloc(list->as.list.elements, 
            sizeof(Value) * list->as.list.capacity);
    }
    list->as.list.elements[list->as.list.count++] = value;
}

static Value make_map() {
    Value value = {VAL_MAP, {.map = {NULL, NULL, 0}}};
    return value;
}

static Value make_function(Node* params, Node* body, Environment* closure) {
    Value value = {VAL_FUNCTION, {.function = {params, body, closure}}};
    return value;
}

static const char* node_type_str(NodeType type) {
    switch (type) {
        case NODE_BLOCK: return "NODE_BLOCK";
        case NODE_LITERAL: return "NODE_LITERAL";
        case NODE_VARIABLE: return "NODE_VARIABLE";
        case NODE_ASSIGNMENT: return "NODE_ASSIGNMENT";
        case NODE_BINARY: return "NODE_BINARY";
        case NODE_UNARY: return "NODE_UNARY";
        case NODE_PRINT: return "NODE_PRINT";
        case NODE_IF: return "NODE_IF";
        case NODE_WHILE: return "NODE_WHILE";
        case NODE_FOR: return "NODE_FOR";
        case NODE_SWITCH: return "NODE_SWITCH";
        case NODE_CASE: return "NODE_CASE";
        case NODE_TRY: return "NODE_TRY";
        case NODE_CATCH: return "NODE_CATCH";
        case NODE_FUNCTION: return "NODE_FUNCTION";
        case NODE_CALL: return "NODE_CALL";
        case NODE_RETURN: return "NODE_RETURN";
        case NODE_VAR_DECL: return "NODE_VAR_DECL";
        case NODE_CONST_DECL: return "NODE_CONST_DECL";
        case NODE_LIST: return "NODE_LIST";
        case NODE_MAP: return "NODE_MAP";
        case NODE_NONE: return "NODE_NONE";
        default: return "UNKNOWN";
    }
}

static void set_error(const char* message) {
    has_error = true;
    if (error_message) free(error_message);
    error_message = strdup(message);
}

static Value evaluate(Node* node) {
    if (!node) return make_none();
    
    switch (node->type) {
        case NODE_BLOCK: {
            Value result = make_none();
            Node* stmt = node->left;
            while (stmt) {
                result = evaluate(stmt);
                stmt = stmt->right;
            }
            return result;
        }
        case NODE_LITERAL:
            if (node->string) {
                return make_string(node->string);
            }
            return make_number(node->number);
        case NODE_VARIABLE:
            return get_variable(current_interpreter, node->value.identifier.start);
        case NODE_BINARY: {
            Value left = evaluate(node->left);
            Value right = evaluate(node->right);
            
            if (left.type != VAL_NUMBER || right.type != VAL_NUMBER) {
                set_error("Operands must be numbers");
                return make_none();
            }
            
            switch (node->value.identifier.type) {
                case TOKEN_PLUS:
                    return make_number(left.as.number + right.as.number);
                case TOKEN_MINUS:
                    return make_number(left.as.number - right.as.number);
                case TOKEN_STAR:
                    return make_number(left.as.number * right.as.number);
                case TOKEN_SLASH:
                    if (right.as.number == 0) {
                        set_error("Division by zero");
                        return make_none();
                    }
                    return make_number(left.as.number / right.as.number);
                case TOKEN_PERCENT:
                    if (right.as.number == 0) {
                        set_error("Modulo by zero");
                        return make_none();
                    }
                    return make_number(fmod(left.as.number, right.as.number));
                case TOKEN_LESS:
                    return make_bool(left.as.number < right.as.number);
                case TOKEN_LESS_EQUAL:
                    return make_bool(left.as.number <= right.as.number);
                case TOKEN_GREATER:
                    return make_bool(left.as.number > right.as.number);
                case TOKEN_GREATER_EQUAL:
                    return make_bool(left.as.number >= right.as.number);
                case TOKEN_EQUAL:
                    return make_bool(left.as.number == right.as.number);
                case TOKEN_BANG_EQUAL:
                    return make_bool(left.as.number != right.as.number);
                default:
                    fprintf(stderr, "Error: Invalid binary operator\n");
                    exit(1);
            }
        }
        case NODE_UNARY: {
            Value right = evaluate(node->right);
            switch (node->value.identifier.type) {
                case TOKEN_MINUS:
                    return make_number(-right.as.number);
                case TOKEN_BANG:
                    return make_bool(!right.as.number);
                default:
                    fprintf(stderr, "Error: Invalid unary operator\n");
                    exit(1);
            }
        }
        case NODE_PRINT: {
            Value value = evaluate(node->left);
            if (value.type == VAL_STRING) {
                const char* str = value.as.string;
                size_t len = strlen(str);
                if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
                    printf("%.*s\n", (int)(len-2), str+1);
                } else {
                    printf("%s\n", str);
                }
            } else {
                printf("%g\n", value.as.number);
            }
            return make_none();
        }
        case NODE_VAR_DECL: {
            Value value = make_none();
            if (node->right) {
                value = evaluate(node->right);
            }
            const char* name = node->left->value.identifier.start;
            set_variable(current_interpreter, name, value);
            return value;
        }
        case NODE_CONST_DECL: {
            Value value = evaluate(node->right);
            const char* name = node->left->value.identifier.start;
            set_variable(current_interpreter, name, value);
            return value;
        }
        case NODE_ASSIGNMENT: {
            Value value = evaluate(node->right);
            const char* name = node->left->value.identifier.start;
            set_variable(current_interpreter, name, value);
            return value;
        }
        case NODE_IF: {
            Value cond = evaluate(node->left);
            bool is_true = (cond.type == VAL_BOOL) ? cond.as.boolean : (cond.type == VAL_NUMBER && cond.as.number != 0);
            
            if (is_true) {
                return evaluate(node->right);
            } else {
                Node* else_block = node->right->right;
                if (!else_block) return make_none();
                return evaluate(else_block);
            }
        }
        case NODE_WHILE: {
            Value result = make_none();
            while (evaluate(node->left).as.number) {
                result = evaluate(node->right);
            }
            return result;
        }
        case NODE_FOR: {
            // Initialize loop variable
            Value init = evaluate(node->left);
            set_variable(current_interpreter, node->value.for_stmt.var_name, init);
            
            Value result = make_none();
            while (1) {
                // Check condition
                Value cond = evaluate(node->value.for_stmt.condition);
                if (!cond.as.number) break;
                
                // Execute body
                result = evaluate(node->value.for_stmt.body);
                
                // Update loop variable
                Value update = evaluate(node->value.for_stmt.update);
                set_variable(current_interpreter, node->value.for_stmt.var_name, update);
            }
            return result;
        }
        case NODE_FOR_IN: {
            Value list = evaluate(node->value.for_in.list);
            if (list.type != VAL_LIST) {
                fprintf(stderr, "Error: Expected list in for-in loop\n");
                exit(1);
            }
            
            Value result = make_none();
            for (size_t i = 0; i < list.as.list.count; i++) {
                set_variable(current_interpreter, node->value.for_in.var_name, list.as.list.elements[i]);
                result = evaluate(node->value.for_in.body);
            }
            return result;
        }
        case NODE_FUNCTION: {
            return make_function(node->value.function.params, node->value.function.body, current_env);
        }
        case NODE_CALL: {
            Value callee = evaluate(node->value.call.callee);
            if (callee.type != VAL_FUNCTION) {
                fprintf(stderr, "Error: Can only call functions\n");
                exit(1);
            }
            
            // Create new environment for function scope
            Environment* prev_env = current_env;
            current_env = new_environment(callee.as.function.closure);
            
            // Evaluate arguments
            Value* args = NULL;
            int arg_count = 0;
            Node* arg = node->value.call.args;
            while (arg) {
                args = realloc(args, sizeof(Value) * (arg_count + 1));
                args[arg_count++] = evaluate(arg);
                arg = arg->right;
            }
            
            // Bind parameters to arguments
            Node* param = callee.as.function.params;
            for (int i = 0; i < arg_count; i++) {
                if (!param) {
                    fprintf(stderr, "Error: Too many arguments\n");
                    exit(1);
                }
                set_variable(current_interpreter, param->value.identifier.start, args[i]);
                param = param->right;
            }
            
            if (param) {
                fprintf(stderr, "Error: Not enough arguments\n");
                exit(1);
            }
            
            // Execute function body
            has_return = false;
            Value result = evaluate(callee.as.function.body);
            
            // Clean up
            free(args);
            Environment* temp = current_env;
            current_env = prev_env;
            free(temp);
            
            if (has_return) {
                return return_value;
            }
            return result;
        }
        case NODE_RETURN: {
            has_return = true;
            if (node->left) {
                return_value = evaluate(node->left);
            } else {
                return_value = make_none();
            }
            return make_none();
        }
        case NODE_SWITCH: {
            Value value = evaluate(node->value.switch_stmt.value);
            Node* case_node = node->value.switch_stmt.cases;
            bool matched = false;
            
            while (case_node) {
                if (case_node->type == NODE_CASE) {
                    if (case_node->value.case_stmt.condition) {
                        Value case_value = evaluate(case_node->value.case_stmt.condition);
                        if (value.type == case_value.type) {
                            if ((value.type == VAL_NUMBER && value.as.number == case_value.as.number) ||
                                (value.type == VAL_STRING && strcmp(value.as.string, case_value.as.string) == 0) ||
                                (value.type == VAL_BOOL && value.as.boolean == case_value.as.boolean)) {
                                matched = true;
                                return evaluate(case_node->value.case_stmt.body);
                            }
                        }
                    } else {
                        // Default case
                        matched = true;
                        return evaluate(case_node->value.case_stmt.body);
                    }
                }
                case_node = case_node->right;
            }
            
            if (!matched) {
                return make_none();
            }
        }
        case NODE_TRY: {
            has_error = false;
            if (error_message) {
                free(error_message);
                error_message = NULL;
            }
            
            // Execute try block
            Value result = evaluate(node->left);
            
            if (has_error) {
                // Execute catch block with error message
                if (node->value.try_stmt.catch_var) {
                    Value error_value = make_string(error_message);
                    set_variable(current_interpreter, node->value.try_stmt.catch_var, error_value);
                }
                result = evaluate(node->value.try_stmt.catch_block);
                has_error = false;
                if (error_message) {
                    free(error_message);
                    error_message = NULL;
                }
            }
            
            return result;
        }
        case NODE_LIST: {
            Value list = make_list();
            Node* element = node->left;
            while (element) {
                Value value = evaluate(element);
                list_add(&list, value);
                element = element->right;
            }
            return list;
        }
        default:
            fprintf(stderr, "Error: Unknown node type\n");
            exit(1);
    }
}

void interpret(Node* node) {
    Interpreter interp;
    current_interpreter = &interp;
    interp.current_scope = new_environment(NULL);
    evaluate(node);
    
    // Clean up global environment
    for (size_t i = 0; i < interp.current_scope->count; i++) {
        free((void*)interp.current_scope->variables[i].name);
        free_value(interp.current_scope->variables[i].value);
    }
    free(interp.current_scope->variables);
    free(interp.current_scope);
}

void free_ast(Node* node) {
    if (node == NULL) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

void init_interpreter(Interpreter* interpreter) {
    interpreter->error = NULL;
    interpreter->should_return = false;
    interpreter->should_break = false;
    interpreter->should_continue = false;
    interpreter->return_value = (Value){.type = VAL_NONE};
    
    // Initialize global scope
    interpreter->current_scope = &interpreter->global_scope;
    interpreter->global_scope.parent = NULL;
    interpreter->global_scope.variables = NULL;
    interpreter->global_scope.count = 0;
    interpreter->global_scope.capacity = 0;
    // No built-in print registration needed
}

Value call_function(Interpreter* interpreter, Function func, Value* args, int arg_count) {
    // ... existing code for non-builtin functions ...
} 