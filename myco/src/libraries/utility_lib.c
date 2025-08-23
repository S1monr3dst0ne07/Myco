/**
 * @file utility_lib.c
 * @brief Myco Utility Library - Debug, type checking, and utility functions
 * @version 1.6.0
 * @author Myco Development Team
 * 
 * This library provides utility functions including:
 * - Debugging and inspection tools
 * - Type checking and validation
 * - String and data utilities
 * - Object and array utilities
 * - Development and testing helpers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "memory_tracker.h"

// Forward declarations
static long long util_debug(ASTNode* args_node);
static long long util_type(ASTNode* args_node);
static long long util_is_num(ASTNode* args_node);
static long long util_is_str(ASTNode* args_node);
static long long util_is_arr(ASTNode* args_node);
static long long util_is_obj(ASTNode* args_node);
static long long util_str(ASTNode* args_node);
static long long util_find(ASTNode* args_node);
static long long util_copy(ASTNode* args_node);
static long long util_has(ASTNode* args_node);
static long long util_len(ASTNode* args_node);
static long long util_first(ASTNode* args_node);
static long long util_last(ASTNode* args_node);
static long long util_push(ASTNode* args_node);
static long long util_pop(ASTNode* args_node);
static long long util_reverse(ASTNode* args_node);

// Helper function to check if string is a valid number
static int is_valid_number(const char* str) {
    if (!str || strlen(str) == 0) return 0;
    
    char* endptr;
    strtoll(str, &endptr, 10);
    return *endptr == '\0';
}

// Helper function to check if string is a valid float
static int is_valid_float(const char* str) {
    if (!str || strlen(str) == 0) return 0;
    
    char* endptr;
    strtod(str, &endptr);
    return *endptr == '\0';
}

// Utility Library Functions
long long call_util_function(const char* func_name, ASTNode* args_node) {
    if (strcmp(func_name, "debug") == 0) {
        return util_debug(args_node);
    } else if (strcmp(func_name, "type") == 0) {
        return util_type(args_node);
    } else if (strcmp(func_name, "is_num") == 0) {
        return util_is_num(args_node);
    } else if (strcmp(func_name, "is_str") == 0) {
        return util_is_str(args_node);
    } else if (strcmp(func_name, "is_arr") == 0) {
        return util_is_arr(args_node);
    } else if (strcmp(func_name, "is_obj") == 0) {
        return util_is_obj(args_node);
    } else if (strcmp(func_name, "str") == 0) {
        return util_str(args_node);
    } else if (strcmp(func_name, "find") == 0) {
        return util_find(args_node);
    } else if (strcmp(func_name, "copy") == 0) {
        return util_copy(args_node);
    } else if (strcmp(func_name, "has") == 0) {
        return util_has(args_node);
    } else if (strcmp(func_name, "len") == 0) {
        return util_len(args_node);
    } else if (strcmp(func_name, "first") == 0) {
        return util_first(args_node);
    } else if (strcmp(func_name, "last") == 0) {
        return util_last(args_node);
    } else if (strcmp(func_name, "push") == 0) {
        return util_push(args_node);
    } else if (strcmp(func_name, "pop") == 0) {
        return util_pop(args_node);
    } else if (strcmp(func_name, "reverse") == 0) {
        return util_reverse(args_node);
    } else {
        fprintf(stderr, "Error: Unknown utility function '%s'\n", func_name);
        return 0;
    }
}

// Utility function implementations
static long long util_debug(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.debug() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.debug() argument must be a valid expression\n");
        return 0;
    }
    
    printf("🔍 DEBUG: %s\n", value_node->text);
    
    // Try to determine the type and provide more information
    if (is_valid_number(value_node->text)) {
        printf("   Type: Number (Integer)\n");
        printf("   Value: %s\n", value_node->text);
    } else if (is_valid_float(value_node->text)) {
        printf("   Type: Number (Float)\n");
        printf("   Value: %s\n", value_node->text);
    } else if (value_node->text[0] == '"' && value_node->text[strlen(value_node->text) - 1] == '"') {
        printf("   Type: String\n");
        printf("   Value: %s\n", value_node->text);
    } else if (strcmp(value_node->text, "True") == 0 || strcmp(value_node->text, "False") == 0) {
        printf("   Type: Boolean\n");
        printf("   Value: %s\n", value_node->text);
    } else {
        printf("   Type: Unknown\n");
        printf("   Value: %s\n", value_node->text);
    }
    
    return 1;
}

static long long util_type(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.type() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.type() argument must be a valid expression\n");
        return 0;
    }
    
    if (is_valid_number(value_node->text)) {
        return 1; // Number
    } else if (is_valid_float(value_node->text)) {
        return 2; // Float
    } else if (value_node->text[0] == '"' && value_node->text[strlen(value_node->text) - 1] == '"') {
        return 3; // String
    } else if (strcmp(value_node->text, "True") == 0 || strcmp(value_node->text, "False") == 0) {
        return 4; // Boolean
    } else {
        return 0; // Unknown
    }
}

static long long util_is_num(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.is_num() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.is_num() argument must be a valid expression\n");
        return 0;
    }
    
    return (is_valid_number(value_node->text) || is_valid_float(value_node->text)) ? 1 : 0;
}

static long long util_is_str(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.is_str() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.is_str() argument must be a valid expression\n");
        return 0;
    }
    
    return (value_node->text[0] == '"' && value_node->text[strlen(value_node->text) - 1] == '"') ? 1 : 0;
}

static long long util_is_arr(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.is_arr() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.is_arr() argument must be a valid expression\n");
        return 0;
    }
    
    // Check if it's an array literal
    return (value_node->text[0] == '[' && value_node->text[strlen(value_node->text) - 1] == ']') ? 1 : 0;
}

static long long util_is_obj(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.is_obj() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.is_obj() argument must be a valid expression\n");
        return 0;
    }
    
    // Check if it's an object literal
    return (value_node->text[0] == '{' && value_node->text[strlen(value_node->text) - 1] == '}') ? 1 : 0;
}

static long long util_str(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.str() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.str() argument must be a valid expression\n");
        return 0;
    }
    
    // Convert to string representation
    printf("String representation: %s\n", value_node->text);
    return 1;
}

static long long util_find(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: util.find() requires two arguments (array, value)\n");
        return 0;
    }
    
    ASTNode* array_node = &args_node->children[0];
    ASTNode* value_node = &args_node->children[1];
    
    if (array_node->type != AST_EXPR || !array_node->text) {
        fprintf(stderr, "Error: util.find() first argument must be a valid expression\n");
        return 0;
    }
    
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.find() second argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Finding %s in array %s\n", value_node->text, array_node->text);
    return 0; // Not found
}

static long long util_copy(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.copy() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.copy() argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Copying: %s\n", value_node->text);
    return 1;
}

static long long util_has(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: util.has() requires two arguments (object, property)\n");
        return 0;
    }
    
    ASTNode* object_node = &args_node->children[0];
    ASTNode* property_node = &args_node->children[1];
    
    if (object_node->type != AST_EXPR || !object_node->text) {
        fprintf(stderr, "Error: util.has() first argument must be a valid expression\n");
        return 0;
    }
    
    if (property_node->type != AST_EXPR || !property_node->text) {
        fprintf(stderr, "Error: util.has() second argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Checking if %s has property %s\n", object_node->text, property_node->text);
    return 0; // Not found
}

static long long util_len(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.len() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.len() argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Getting length of: %s\n", value_node->text);
    return 0;
}

static long long util_first(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.first() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.first() argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Getting first element of: %s\n", value_node->text);
    return 0;
}

static long long util_last(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.last() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.last() argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Getting last element of: %s\n", value_node->text);
    return 0;
}

static long long util_push(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: util.push() requires two arguments (array, value)\n");
        return 0;
    }
    
    ASTNode* array_node = &args_node->children[0];
    ASTNode* value_node = &args_node->children[1];
    
    if (array_node->type != AST_EXPR || !array_node->text) {
        fprintf(stderr, "Error: util.push() first argument must be a valid expression\n");
        return 0;
    }
    
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: util.push() second argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Pushing %s to array %s\n", value_node->text, array_node->text);
    return 1;
}

static long long util_pop(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.pop() requires one argument\n");
        return 0;
    }
    
    ASTNode* array_node = &args_node->children[0];
    if (array_node->type != AST_EXPR || !array_node->text) {
        fprintf(stderr, "Error: util.pop() argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Popping from array: %s\n", array_node->text);
    return 0;
}

static long long util_reverse(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: util.reverse() requires one argument\n");
        return 0;
    }
    
    ASTNode* array_node = &args_node->children[0];
    if (array_node->type != AST_EXPR || !array_node->text) {
        fprintf(stderr, "Error: util.reverse() argument must be a valid expression\n");
        return 0;
    }
    
    // For now, return a placeholder implementation
    printf("Reversing array: %s\n", array_node->text);
    return 1;
}
