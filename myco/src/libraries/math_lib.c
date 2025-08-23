/**
 * @file math_lib.c
 * @brief Myco Math Library - Mathematical functions and constants
 * @version 1.6.0
 * @author Myco Development Team
 * 
 * This library provides comprehensive mathematical functions including:
 * - Basic operations (abs, pow, sqrt, floor, ceil)
 * - Trigonometric functions (sin, cos, tan)
 * - Comparison functions (min, max)
 * - Random number generation
 * - Mathematical constants (PI, E, INF, NAN)
 * - Float support with full arithmetic operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "parser.h"
#include "memory_tracker.h"

// Forward declarations
static long long math_abs(ASTNode* args_node);
static long long math_pow(ASTNode* args_node);
static long long math_sqrt(ASTNode* args_node);
static long long math_floor(ASTNode* args_node);
static long long math_ceil(ASTNode* args_node);
static long long math_sin(ASTNode* args_node);
static long long math_cos(ASTNode* args_node);
static long long math_tan(ASTNode* args_node);
static long long math_min(ASTNode* args_node);
static long long math_max(ASTNode* args_node);
static long long math_random(ASTNode* args_node);
static long long math_randint(ASTNode* args_node);
static long long math_choice(ASTNode* args_node);
static long long math_get_constant(ASTNode* args_node);

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

// Helper function to get numeric value from AST node
static long long get_numeric_value(ASTNode* node) {
    if (!node || !node->text) return 0;
    
    if (is_valid_number(node->text)) {
        return strtoll(node->text, NULL, 10);
    } else if (is_valid_float(node->text)) {
        return (long long)(strtod(node->text, NULL) * 1000000); // Scale for precision
    }
    
    return 0;
}

// Helper function to get float value from AST node
static double get_float_value(ASTNode* node) {
    if (!node || !node->text) return 0.0;
    
    if (is_valid_float(node->text)) {
        return strtod(node->text, NULL);
    } else if (is_valid_number(node->text)) {
        return (double)strtoll(node->text, NULL, 10);
    }
    
    return 0.0;
}

// Math Library Functions
long long call_math_function(const char* func_name, ASTNode* args_node) {
    if (strcmp(func_name, "abs") == 0) {
        return math_abs(args_node);
    } else if (strcmp(func_name, "pow") == 0) {
        return math_pow(args_node);
    } else if (strcmp(func_name, "sqrt") == 0) {
        return math_sqrt(args_node);
    } else if (strcmp(func_name, "floor") == 0) {
        return math_floor(args_node);
    } else if (strcmp(func_name, "ceil") == 0) {
        return math_ceil(args_node);
    } else if (strcmp(func_name, "sin") == 0) {
        return math_sin(args_node);
    } else if (strcmp(func_name, "cos") == 0) {
        return math_cos(args_node);
    } else if (strcmp(func_name, "tan") == 0) {
        return math_tan(args_node);
    } else if (strcmp(func_name, "min") == 0) {
        return math_min(args_node);
    } else if (strcmp(func_name, "max") == 0) {
        return math_max(args_node);
    } else if (strcmp(func_name, "random") == 0) {
        return math_random(args_node);
    } else if (strcmp(func_name, "randint") == 0) {
        return math_randint(args_node);
    } else if (strcmp(func_name, "choice") == 0) {
        return math_choice(args_node);
    } else if (strcmp(func_name, "get_constant") == 0) {
        return math_get_constant(args_node);
    } else {
        fprintf(stderr, "Error: Unknown math function '%s'\n", func_name);
        return 0;
    }
}

// Math function implementations
static long long math_abs(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.abs() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.abs() argument must be a valid expression\n");
        return 0;
    }
    
    if (is_valid_float(value_node->text)) {
        double float_val = strtod(value_node->text, NULL);
        return (long long)(fabs(float_val) * 1000000);
    } else if (is_valid_number(value_node->text)) {
        long long int_val = strtoll(value_node->text, NULL, 10);
        return llabs(int_val);
    } else {
        fprintf(stderr, "Error: math.abs() argument must be a valid number\n");
        return 0;
    }
}

static long long math_pow(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: math.pow() requires two arguments (base, exponent)\n");
        return 0;
    }
    
    ASTNode* base_node = &args_node->children[0];
    ASTNode* exp_node = &args_node->children[1];
    
    if (base_node->type != AST_EXPR || !base_node->text) {
        fprintf(stderr, "Error: math.pow() first argument must be a valid expression\n");
        return 0;
    }
    
    if (exp_node->type != AST_EXPR || !exp_node->text) {
        fprintf(stderr, "Error: math.pow() second argument must be a valid expression\n");
        return 0;
    }
    
    double base = get_float_value(base_node);
    double exp = get_float_value(exp_node);
    double result = pow(base, exp);
    
    return (long long)(result * 1000000);
}

static long long math_sqrt(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.sqrt() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.sqrt() argument must be a valid expression\n");
        return 0;
    }
    
    double value = get_float_value(value_node);
    if (value < 0) {
        fprintf(stderr, "Error: math.sqrt() argument must be non-negative\n");
        return 0;
    }
    
    double result = sqrt(value);
    return (long long)(result * 1000000);
}

static long long math_floor(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.floor() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.floor() argument must be a valid expression\n");
        return 0;
    }
    
    double value = get_float_value(value_node);
    double result = floor(value);
    return (long long)(result * 1000000);
}

static long long math_ceil(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.ceil() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.ceil() argument must be a valid expression\n");
        return 0;
    }
    
    double value = get_float_value(value_node);
    double result = ceil(value);
    return (long long)(result * 1000000);
}

static long long math_sin(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.sin() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.sin() argument must be a valid expression\n");
        return 0;
    }
    
    double value = get_float_value(value_node);
    double result = sin(value);
    return (long long)(result * 1000000);
}

static long long math_cos(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.cos() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.cos() argument must be a valid expression\n");
        return 0;
    }
    
    double value = get_float_value(value_node);
    double result = cos(value);
    return (long long)(result * 1000000);
}

static long long math_tan(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.tan() requires one argument\n");
        return 0;
    }
    
    ASTNode* value_node = &args_node->children[0];
    if (value_node->type != AST_EXPR || !value_node->text) {
        fprintf(stderr, "Error: math.tan() argument must be a valid expression\n");
        return 0;
    }
    
    double value = get_float_value(value_node);
    double result = tan(value);
    return (long long)(result * 1000000);
}

static long long math_min(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: math.min() requires at least two arguments\n");
        return 0;
    }
    
    double min_val = INFINITY;
    
    for (int i = 0; i < args_node->child_count; i++) {
        ASTNode* child = &args_node->children[i];
        if (child->type != AST_EXPR || !child->text) {
            fprintf(stderr, "Error: math.min() argument %d must be a valid expression\n", i + 1);
            return 0;
        }
        
        double value = get_float_value(child);
        if (value < min_val) {
            min_val = value;
        }
    }
    
    return (long long)(min_val * 1000000);
}

static long long math_max(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: math.max() requires at least two arguments\n");
        return 0;
    }
    
    double max_val = -INFINITY;
    
    for (int i = 0; i < args_node->child_count; i++) {
        ASTNode* child = &args_node->children[i];
        if (child->type != AST_EXPR || !child->text) {
            fprintf(stderr, "Error: math.max() argument %d must be a valid expression\n", i + 1);
            return 0;
        }
        
        double value = get_float_value(child);
        if (value > max_val) {
            max_val = value;
        }
    }
    
    return (long long)(max_val * 1000000);
}

static long long math_random(ASTNode* args_node) {
    if (args_node->child_count != 0) {
        fprintf(stderr, "Error: math.random() takes no arguments\n");
        return 0;
    }
    
    double random_val = ((double)rand()) / RAND_MAX;
    return (long long)(random_val * 1000000);
}

static long long math_randint(ASTNode* args_node) {
    if (args_node->child_count < 2) {
        fprintf(stderr, "Error: math.randint() requires two arguments (min, max)\n");
        return 0;
    }
    
    ASTNode* min_node = &args_node->children[0];
    ASTNode* max_node = &args_node->children[1];
    
    if (min_node->type != AST_EXPR || !min_node->text) {
        fprintf(stderr, "Error: math.randint() first argument must be a valid expression\n");
        return 0;
    }
    
    if (max_node->type != AST_EXPR || !max_node->text) {
        fprintf(stderr, "Error: math.randint() second argument must be a valid expression\n");
        return 0;
    }
    
    long long min_val = get_numeric_value(min_node);
    long long max_val = get_numeric_value(max_node);
    
    if (min_val > max_val) {
        long long temp = min_val;
        min_val = max_val;
        max_val = temp;
    }
    
    long long range = max_val - min_val + 1;
    long long random_val = min_val + (rand() % range);
    
    return random_val;
}

static long long math_choice(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.choice() requires at least one argument\n");
        return 0;
    }
    
    int choice_index = rand() % args_node->child_count;
    ASTNode* chosen_node = &args_node->children[choice_index];
    
    if (chosen_node->type != AST_EXPR || !chosen_node->text) {
        fprintf(stderr, "Error: math.choice() argument %d must be a valid expression\n", choice_index + 1);
        return 0;
    }
    
    return get_numeric_value(chosen_node);
}

static long long math_get_constant(ASTNode* args_node) {
    if (args_node->child_count < 1) {
        fprintf(stderr, "Error: math.get_constant() requires one argument (constant_name)\n");
        return 0;
    }
    
    ASTNode* const_name_node = &args_node->children[0];
    if (const_name_node->type != AST_EXPR || !const_name_node->text) {
        fprintf(stderr, "Error: math.get_constant() argument must be a valid expression\n");
        return 0;
    }
    
    if (strcmp(const_name_node->text, "PI") == 0) {
        return (long long)(M_PI * 1000000);
    } else if (strcmp(const_name_node->text, "E") == 0) {
        return (long long)(M_E * 1000000);
    } else if (strcmp(const_name_node->text, "INF") == 0) {
        return (long long)(INFINITY * 1000000);
    } else if (strcmp(const_name_node->text, "NAN") == 0) {
        return (long long)(NAN * 1000000);
    } else {
        fprintf(stderr, "Error: Unknown mathematical constant '%s'\n", const_name_node->text);
        return 0;
    }
}
