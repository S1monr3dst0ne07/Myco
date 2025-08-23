/**
 * @file eval.c
 * @brief Myco Core Evaluation - Expression evaluation and control flow
 * @version 1.6.0
 * @author Myco Development Team
 * 
 * This file implements the core evaluation logic for the Myco interpreter.
 * It handles expression evaluation, variable management, and control flow.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "memory_tracker.h"
#include "loop_manager.h"
#include "../../include/eval.h"

// Forward declarations
static long long eval_assignment_expression(ASTNode* ast);
static long long eval_primary_expression(ASTNode* ast);
static long long eval_array_access(ASTNode* ast);
static long long eval_object_access(ASTNode* ast);
static long long eval_library_access(ASTNode* ast);

// Helper function declarations
static int is_valid_number(const char* str);
static int is_valid_float(const char* str);
static long long eval_binary_expression(ASTNode* ast);
int is_float_variable(const char* name);

// Function declarations
void set_variable(const char* name, long long value);
long long get_variable(const char* name);
const char* get_string_variable(const char* name);
long long call_user_function(const char* name, ASTNode* ast);
long long call_math_function(const char* func_name, ASTNode* args_node);
long long call_util_function(const char* func_name, ASTNode* args_node);
long long call_file_io_function(const char* func_name, ASTNode* args_node);
long long call_path_utils_function(const char* func_name, ASTNode* args_node);
long long call_env_function(const char* func_name, ASTNode* args_node);
long long call_args_function(const char* func_name, ASTNode* args_node);
long long call_process_function(const char* func_name, ASTNode* args_node);
long long call_text_utils_function(const char* func_name, ASTNode* args_node);
long long call_debug_function(const char* func_name, ASTNode* args_node);
long long call_type_system_function(const char* func_name, ASTNode* args_node);
long long call_language_polish_function(const char* func_name, ASTNode* args_node);
long long call_testing_framework_function(const char* func_name, ASTNode* args_node);
long long call_data_structures_function(const char* func_name, ASTNode* args_node);
const char* get_library_alias(const char* library_name);
MycoArray* get_array_value(const char* name);
MycoObject* get_object_value(const char* name);

// Global command-line arguments storage
static char** global_argv = NULL;
static int global_argc = 0;

// Simple variable storage
#define MAX_VARIABLES 1000
typedef struct {
    char name[64];
    long long value;
    int is_string;
    int is_float;
    char* string_value;
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count = 0;

// Global string for concatenation results
static char* last_concat_result = NULL;

// Global variable to track the type of the last expression result
static int last_result_is_float = 0;

// Function storage
#define MAX_FUNCTIONS 100
typedef struct {
    char name[64];
    ASTNode* definition;
} Function;

static Function functions[MAX_FUNCTIONS];
static int function_count = 0;

// Core evaluation function
long long eval_expression(ASTNode* ast) {
    if (!ast) return 0;
    
    switch (ast->type) {
        case AST_EXPR:
            return eval_primary_expression(ast);
        case AST_ASSIGN:
            return eval_assignment_expression(ast);
        case AST_ARRAY_ACCESS:
            return eval_array_access(ast);
        case AST_OBJECT_ACCESS:
            return eval_object_access(ast);
        case AST_DOT:
            return eval_library_access(ast);
        default:
            fprintf(stderr, "Error: Unknown AST node type: %d\n", ast->type);
            return 0;
    }
}





// Assignment expression evaluation
static long long eval_assignment_expression(ASTNode* ast) {
    if (!ast || ast->child_count < 2) return 0;
    
    ASTNode* target = &ast->children[0];
    long long value = 0; // TODO: Implement proper evaluation
    
    if (target->type != AST_EXPR || !target->text) {
        fprintf(stderr, "Error: Invalid assignment target\n");
        return 0;
    }
    
    // Set variable in global environment
    set_variable(target->text, value);
    
    return value;
}





// Forward declarations
static long long eval_function_call(ASTNode* ast);
static ASTNode* find_function(const char* name);
static long long eval_statement(ASTNode* ast);

// Binary expression evaluation
static long long eval_binary_expression(ASTNode* ast) {
    if (!ast || ast->child_count < 2) return 0;
    
    long long left = eval_expression(&ast->children[0]);
    long long right = eval_expression(&ast->children[1]);
    
    
    
    // Check if operands come from float variables (this is the reliable way)
    int left_is_float = 0;
    int right_is_float = 0;
    
    if (ast->children[0].text && is_float_variable(ast->children[0].text)) {
        left_is_float = 1;
    }
    if (ast->children[1].text && is_float_variable(ast->children[1].text)) {
        right_is_float = 1;
    }
    
    if (strcmp(ast->text, "+") == 0) {
        // Check if either operand is a string (indicated by -1 for variables or 1 for literals)
        if (left == -1 || right == -1 || left == 1 || right == 1) {
            // String concatenation
            char* left_str = NULL;
            char* right_str = NULL;
            
            // Get left string value
            if (left == -1) {
                if (last_concat_result) {
                    left_str = strdup(last_concat_result);
                } else if (ast->children[0].text) {
                    const char* str_val = get_string_variable(ast->children[0].text);
                    if (str_val) {
                        left_str = strdup(str_val);
                    }
                }
            } else if (left == 1) {
                // String literal - extract value from AST
                if (ast->children[0].text && ast->children[0].text[0] == '"') {
                    size_t len = strlen(ast->children[0].text);
                    if (len >= 2) {
                        left_str = malloc(len - 1);
                        strncpy(left_str, ast->children[0].text + 1, len - 2);
                        left_str[len - 2] = '\0';
                    }
                }
            } else {
                // Convert number to string
                char temp[64];
                snprintf(temp, sizeof(temp), "%lld", left);
                left_str = strdup(temp);
            }
            
            // Get right string value
            if (right == -1) {
                if (ast->children[1].text) {
                    const char* str_val = get_string_variable(ast->children[1].text);
                    if (str_val) {
                        right_str = strdup(str_val);
                    }
                }
            } else if (right == 1) {
                // String literal - extract value from AST
                if (ast->children[1].text && ast->children[1].text[0] == '"') {
                    size_t len = strlen(ast->children[1].text);
                    if (len >= 2) {
                        right_str = malloc(len - 1);
                        strncpy(right_str, ast->children[1].text + 1, len - 2);
                        right_str[len - 2] = '\0';
                    }
                }
            } else {
                // Convert number to string
                char temp[64];
                snprintf(temp, sizeof(temp), "%lld", right);
                right_str = strdup(temp);
            }
            
            // Concatenate strings
            if (left_str && right_str) {
                size_t total_len = strlen(left_str) + strlen(right_str) + 1;
                char* result_str = malloc(total_len);
                if (result_str) {
                    strcpy(result_str, left_str);
                    strcat(result_str, right_str);
                    
                    // Store result globally
                    if (last_concat_result) {
                        free(last_concat_result);
                    }
                    last_concat_result = strdup(result_str);
                    
                    // Clean up
                    free(left_str);
                    free(right_str);
                    free(result_str);
                    
                    return -1; // Indicate string result
                }
            }
            
            // Clean up on failure
            if (left_str) free(left_str);
            if (right_str) free(right_str);
            return -1;
        }
        return left + right;
    } else if (strcmp(ast->text, "-") == 0) {
        return left - right;
    } else if (strcmp(ast->text, "*") == 0) {
        if (left_is_float || right_is_float) {
            // Float multiplication: (a/1M) * (b/1M) = (a*b)/(1M*1M) -> scale back by 1M
            double left_val = (double)left / 1000000.0;
            double right_val = (double)right / 1000000.0;
            double result = left_val * right_val;
            last_result_is_float = 1; // Mark result as float
            return (long long)(result * 1000000.0);
        }
        last_result_is_float = 0; // Mark result as integer
        return left * right;
    } else if (strcmp(ast->text, "/") == 0) {
        if (right == 0) {
            fprintf(stderr, "Error: Division by zero\n");
            return 0;
        }
        if (left_is_float || right_is_float) {
            // Float division: (a/1M) / (b/1M) = a/b -> no scaling needed
            double left_val = (double)left / 1000000.0;
            double right_val = (double)right / 1000000.0;
            double result = left_val / right_val;
            last_result_is_float = 1; // Mark result as float
            return (long long)(result * 1000000.0);
        }
        last_result_is_float = 0; // Mark result as integer
        return left / right;
    } else if (strcmp(ast->text, "%") == 0) {
        if (right == 0) {
            fprintf(stderr, "Error: Modulo by zero\n");
            return 0;
        }
        return left % right;
    } else if (strcmp(ast->text, "==") == 0) {
        return (left == right) ? 1 : 0;
    } else if (strcmp(ast->text, "!=") == 0) {
        return (left != right) ? 1 : 0;
    } else if (strcmp(ast->text, "<") == 0) {
        return (left < right) ? 1 : 0;
    } else if (strcmp(ast->text, ">") == 0) {
        return (left > right) ? 1 : 0;
    } else if (strcmp(ast->text, "<=") == 0) {
        return (left <= right) ? 1 : 0;
    } else if (strcmp(ast->text, ">=") == 0) {
        return (left >= right) ? 1 : 0;
    }
    
    fprintf(stderr, "Error: Unknown binary operator: %s\n", ast->text);
    return 0;
}

// Primary expression evaluation
static long long eval_primary_expression(ASTNode* ast) {
    if (!ast || !ast->text) return 0;
    
    // Check if it's a function call
    if (ast->text && strcmp(ast->text, "call") == 0 && ast->child_count >= 1) {
        return eval_function_call(ast);
    }
    
    // Check if it's a binary operator
    if (ast->child_count >= 2 && (
        strcmp(ast->text, "+") == 0 || strcmp(ast->text, "-") == 0 ||
        strcmp(ast->text, "*") == 0 || strcmp(ast->text, "/") == 0 ||
        strcmp(ast->text, "%") == 0 || strcmp(ast->text, "==") == 0 ||
        strcmp(ast->text, "!=") == 0 || strcmp(ast->text, "<") == 0 ||
        strcmp(ast->text, ">") == 0 || strcmp(ast->text, "<=") == 0 ||
        strcmp(ast->text, ">=") == 0)) {
        return eval_binary_expression(ast);
    }
    
    // Check if it's a number
    if (is_valid_number(ast->text)) {
        return strtoll(ast->text, NULL, 10);
    }
    
    // Check if it's a float
    if (is_valid_float(ast->text)) {
        return (long long)(strtod(ast->text, NULL) * 1000000);
    }
    
    // Check if it's a string literal
    if (ast->text[0] == '"' && ast->text[strlen(ast->text) - 1] == '"') {
        // Return 1 to indicate string literal
        return 1;
    }
    
    // Check if it's a boolean
    if (strcmp(ast->text, "True") == 0) {
        return 1;
    } else if (strcmp(ast->text, "False") == 0) {
        return 0;
    }
    
    // Check if it's a string variable first
    const char* str_value = get_string_variable(ast->text);
    if (str_value) {
        // Return a special value to indicate this is a string
        return -1;
    }
    
    // Check if it's a numeric variable
    long long var_value = get_variable(ast->text);
    if (var_value != 0 || strcmp(ast->text, "0") == 0) {
        return var_value;
    }
    
    // Unknown expression
    fprintf(stderr, "Error: Unknown expression: %s\n", ast->text);
    return 0;
}

// Array access evaluation
static long long eval_array_access(ASTNode* ast) {
    if (!ast || ast->child_count < 2) return 0;
    
    ASTNode* array_name_node = &ast->children[0];
    ASTNode* index_node = &ast->children[1];
    
    if (array_name_node->type != AST_EXPR || !array_name_node->text) {
        fprintf(stderr, "Error: Invalid array name\n");
        return 0;
    }
    
    long long index = 0; // TODO: Implement proper evaluation
    
    // Get array from environment
    MycoArray* array = get_array_value(array_name_node->text);
    if (!array) {
        fprintf(stderr, "Error: Array '%s' not found\n", array_name_node->text);
        return 0;
    }
    
    if (index < 0 || index >= array->size) {
        fprintf(stderr, "Error: Array index out of bounds: %lld\n", index);
        return 0;
    }
    
    if (array->is_string_array) {
        return (long long)array->str_elements[index];
    } else {
        return array->elements[index];
    }
}

// Object access evaluation
static long long eval_object_access(ASTNode* ast) {
    if (!ast || ast->child_count < 2) return 0;
    
    ASTNode* object_name_node = &ast->children[0];
    ASTNode* property_name_node = &ast->children[1];
    
    if (object_name_node->type != AST_EXPR || !object_name_node->text) {
        fprintf(stderr, "Error: Invalid object name\n");
        return 0;
    }
    
    if (property_name_node->type != AST_EXPR || !property_name_node->text) {
        fprintf(stderr, "Error: Invalid property name\n");
        return 0;
    }
    
    // Get object from environment
    MycoObject* obj = get_object_value(object_name_node->text);
    if (!obj) {
        fprintf(stderr, "Error: Object '%s' not found\n", object_name_node->text);
        return 0;
    }
    
    // Get property value
    void* property_value = object_get_property(obj, property_name_node->text);
    if (!property_value) {
        fprintf(stderr, "Error: Property '%s' not found in object '%s'\n", 
                property_name_node->text, object_name_node->text);
        return 0;
    }
    
    PropertyType prop_type = object_get_property_type(obj, property_name_node->text);
    
    switch (prop_type) {
        case PROP_TYPE_NUMBER:
            return (long long)property_value;
        case PROP_TYPE_STRING:
            return (long long)property_value;
        case PROP_TYPE_OBJECT:
            return (long long)property_value;
        default:
            fprintf(stderr, "Error: Unknown property type\n");
            return 0;
    }
}

// Library access evaluation
static long long eval_library_access(ASTNode* ast) {
    if (!ast || ast->child_count < 2) return 0;
    
    ASTNode* library_name_node = &ast->children[0];
    ASTNode* function_name_node = &ast->children[1];
    
    if (library_name_node->type != AST_EXPR || !library_name_node->text) {
        fprintf(stderr, "Error: Invalid library name\n");
        return 0;
    }
    
    if (function_name_node->type != AST_EXPR || !function_name_node->text) {
        fprintf(stderr, "Error: Invalid function name\n");
        return 0;
    }
    
    const char* library_name = library_name_node->text;
    const char* function_name = function_name_node->text;
    
    // Get library alias
    const char* actual_library = get_library_alias(library_name);
    if (!actual_library) {
        fprintf(stderr, "Error: Library '%s' not imported\n", library_name);
        return 0;
    }
    
    // Dispatch to appropriate library function
    if (strcmp(actual_library, "math") == 0) {
        return call_math_function(function_name, ast);
    } else if (strcmp(actual_library, "util") == 0) {
        return call_util_function(function_name, ast);
    } else if (strcmp(actual_library, "io") == 0) {
        return call_file_io_function(function_name, ast);
    } else if (strcmp(actual_library, "path_utils") == 0) {
        return call_path_utils_function(function_name, ast);
    } else if (strcmp(actual_library, "env") == 0) {
        return call_env_function(function_name, ast);
    } else if (strcmp(actual_library, "args") == 0) {
        return call_args_function(function_name, ast);
    } else if (strcmp(actual_library, "process") == 0) {
        return call_process_function(function_name, ast);
    } else if (strcmp(actual_library, "text_utils") == 0) {
        return call_text_utils_function(function_name, ast);
    } else if (strcmp(actual_library, "debug") == 0) {
        return call_debug_function(function_name, ast);
    } else if (strcmp(actual_library, "types") == 0) {
        return call_type_system_function(function_name, ast);
    } else if (strcmp(actual_library, "polish") == 0) {
        return call_language_polish_function(function_name, ast);
    } else if (strcmp(actual_library, "test") == 0) {
        return call_testing_framework_function(function_name, ast);
    } else if (strcmp(actual_library, "data") == 0) {
        return call_data_structures_function(function_name, ast);
    } else {
        fprintf(stderr, "Error: Unknown library '%s'\n", actual_library);
        return 0;
    }
}

// Helper functions
static int is_valid_number(const char* str) {
    if (!str || strlen(str) == 0) return 0;
    
    char* endptr;
    strtoll(str, &endptr, 10);
    return *endptr == '\0';
}

static int is_valid_float(const char* str) {
    if (!str || strlen(str) == 0) return 0;
    
    char* endptr;
    strtod(str, &endptr);
    return *endptr == '\0';
}

// Variable management functions
void set_variable(const char* name, long long value) {
    // Look for existing variable
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
                    variables[i].value = value;
        variables[i].is_string = 0;
        variables[i].is_float = 0;
        if (variables[i].string_value) {
            free(variables[i].string_value);
            variables[i].string_value = NULL;
        }
            return;
        }
    }
    
    // Add new variable
    if (variable_count < MAX_VARIABLES) {
        strncpy(variables[variable_count].name, name, 63);
        variables[variable_count].name[63] = '\0';
        variables[variable_count].value = value;
        variables[variable_count].is_string = 0;
        variables[variable_count].is_float = 0;
        variables[variable_count].string_value = NULL;
        variable_count++;
    }
}

void set_float_variable(const char* name, double value) {
    long long scaled_value = (long long)(value * 1000000);
    
    // Look for existing variable
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            variables[i].value = scaled_value;
            variables[i].is_string = 0;
            variables[i].is_float = 1;
            if (variables[i].string_value) {
                free(variables[i].string_value);
                variables[i].string_value = NULL;
            }
            return;
        }
    }
    
    // Add new variable
    if (variable_count < MAX_VARIABLES) {
        strncpy(variables[variable_count].name, name, 63);
        variables[variable_count].name[63] = '\0';
        variables[variable_count].value = scaled_value;
        variables[variable_count].is_string = 0;
        variables[variable_count].is_float = 1;
        variables[variable_count].string_value = NULL;
        variable_count++;
    }
}

void set_string_variable(const char* name, const char* value) {
    // Look for existing variable
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            variables[i].is_string = 1;
            if (variables[i].string_value) {
                free(variables[i].string_value);
            }
            variables[i].string_value = strdup(value);
            return;
        }
    }
    
    // Add new variable
    if (variable_count < MAX_VARIABLES) {
        strncpy(variables[variable_count].name, name, 63);
        variables[variable_count].name[63] = '\0';
        variables[variable_count].value = 0;
        variables[variable_count].is_string = 1;
        variables[variable_count].string_value = strdup(value);
        variable_count++;
    }
}

long long get_variable(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return 0;
}

const char* get_string_variable(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0 && variables[i].is_string) {
            return variables[i].string_value;
        }
    }
    return NULL;
}

int is_float_variable(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].is_float;
        }
    }
    return 0;
}

// Function call evaluation
static long long eval_function_call(ASTNode* ast) {
    if (!ast || ast->child_count < 1) return 0;
    
    // Get function name from first child
    ASTNode* func_name_node = &ast->children[0];
    if (!func_name_node->text) return 0;
    
    // Look up the function
    ASTNode* func_def = find_function(func_name_node->text);
    if (!func_def) {
        fprintf(stderr, "Error: Function '%s' not found\n", func_name_node->text);
        return 0;
    }
    
    // Find the function body (should be the last child)
    ASTNode* func_body = NULL;
    for (int i = 0; i < func_def->child_count; i++) {
        if (func_def->children[i].type == AST_BLOCK) {
            func_body = &func_def->children[i];
            break;
        }
    }
    
    if (!func_body) {
        fprintf(stderr, "Error: Function '%s' has no body\n", func_name_node->text);
        return 0;
    }
    
    // Simple parameter binding - bind parameters to arguments
    // This is a simplified implementation
    if (ast->child_count > 1 && ast->children[1].child_count > 0) {
        ASTNode* args_node = &ast->children[1];
        int param_index = 0;
        
        // Bind parameters to arguments
        for (int i = 0; i < func_def->child_count - 1 && i < args_node->child_count; i++) {
            // Skip the function body (last child)
            if (func_def->children[i].type == AST_EXPR && func_def->children[i].text) {
                // This is a parameter name
                long long arg_value = eval_expression(&args_node->children[i]);
                set_variable(func_def->children[i].text, arg_value);
                param_index++;
            }
        }
    }
    
    // Execute the function body
    // For now, we'll just evaluate the first statement in the body
    // This is a simplified implementation - we'll need to handle parameters and return values properly
    if (func_body->child_count > 0) {
        return eval_statement(&func_body->children[0]);
    }
    
    return 0;
}

// Statement evaluation for function bodies
static long long eval_statement(ASTNode* ast) {
    if (!ast) return 0;
    
    switch (ast->type) {
        case AST_RETURN:
            if (ast->child_count > 0) {
                return eval_expression(&ast->children[0]);
            }
            return 0;
            
        case AST_EXPR:
            return eval_expression(ast);
            
        case AST_LET:
            // Handle variable declaration in function scope
            if (ast->child_count >= 2) {
                long long value = eval_expression(&ast->children[1]);
                // For now, just return the value - we'll implement proper scoping later
                return value;
            }
            return 0;
            
        case AST_PRINT:
            // Handle print statements in function scope
            if (ast->child_count > 0) {
                eval_expression(&ast->children[0]);
            }
            return 0;
            
        default:
            // For other statement types, try to evaluate as expression
            return eval_expression(ast);
    }
}

// Function management functions
void register_function(const char* name, ASTNode* definition) {
    if (function_count < MAX_FUNCTIONS) {
        strncpy(functions[function_count].name, name, 63);
        functions[function_count].name[63] = '\0';
        functions[function_count].definition = definition;
        function_count++;
    }
}

static ASTNode* find_function(const char* name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            return functions[i].definition;
        }
    }
    return NULL;
}

long long call_user_function(const char* name, ASTNode* ast) {
    // Implementation for calling user-defined functions
    // This will be implemented in functions.c
    return 0;
}

// Built-in function evaluations (placeholders for now)
long long eval_print_function(ASTNode* ast) { return 0; }
long long eval_len_function(ASTNode* ast) { return 0; }
long long eval_first_function(ASTNode* ast) { return 0; }
long long eval_last_function(ASTNode* ast) { return 0; }
long long eval_push_function(ASTNode* ast) { return 0; }
long long eval_pop_function(ASTNode* ast) { return 0; }
long long eval_reverse_function(ASTNode* ast) { return 0; }
long long eval_filter_function(ASTNode* ast) { return 0; }
long long eval_map_function(ASTNode* ast) { return 0; }
long long eval_reduce_function(ASTNode* ast) { return 0; }

// Library import functions (placeholders for now)
const char* get_library_alias(const char* library_name) { return NULL; }

// Get the last concatenation result
const char* get_last_concat_result(void) {
    return last_concat_result;
}

// Get whether the last expression result was a float
int get_last_result_is_float(void) {
    return last_result_is_float;
}
