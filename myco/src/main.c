/**
 * @file main.c
 * @brief Myco Language Interpreter - Main Entry Point
 * @version 1.0.0
 * @author Myco Development Team
 * 
 * This file contains the main entry point for the Myco interpreter.
 * It orchestrates the complete interpretation process from source file
 * to execution, handling command line arguments and coordinating
 * all phases of interpretation.
 * 
 * Program Flow:
 * 1. Command line argument parsing
 * 2. Source file loading and validation
 * 3. Lexical analysis (tokenization)
 * 4. Parsing (AST construction)
 * 5. Execution (AST evaluation)
 * 6. Cleanup and memory management
 * 
 * Command Line Options:
 * - <input_file>: Myco source file to interpret
 * - --build: Generate C output instead of interpreting
 * - --output <file>: Specify output file for build mode
 * 
 * Error Handling:
 * - File I/O errors with descriptive messages
 * - Memory allocation failure handling
 * - Lexical and parsing error propagation
 * - Graceful cleanup on failure
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "codegen.h"
#include "memory_tracker.h"
#include "config.h"

// Forward declarations for variable management
void set_variable(const char* name, long long value);
void set_float_variable(const char* name, double value);
void set_string_variable(const char* name, const char* value);
long long get_variable(const char* name);
const char* get_string_variable(const char* name);
int is_float_variable(const char* name);

// Forward declaration for expression evaluation
long long eval_expression(ASTNode* ast);
const char* get_last_concat_result(void);
int get_last_result_is_float(void);
void register_function(const char* name, ASTNode* definition);

// Placeholder implementations for functions not yet implemented in modular structure
void eval_set_base_dir(const char* dir) {
    // TODO: Implement base directory setting
}

void init_implicit_functions(void) {
    // TODO: Initialize implicit functions
}

void init_libraries(void) {
    // TODO: Initialize libraries
}

void set_command_line_args(int argc, char** argv) {
    // TODO: Set command line arguments
}

void eval_evaluate(ASTNode* ast) {
    if (!ast) return;
    
    switch (ast->type) {
        case AST_PRINT: {
            // Handle print statements
            if (ast->child_count > 0) {
                ASTNode* expr = &ast->children[0];
                
                // Check if it's a binary expression (like "x = " + x)
                if (expr->child_count == 2 && expr->text && strcmp(expr->text, "+") == 0) {
                    // Handle string concatenation
                    char result[512] = "";
                    
                    // Left side
                    ASTNode* left = &expr->children[0];
                    
                    if (left->text && left->child_count == 0) {
                        // Simple variable reference or string literal
                        if (left->text[0] == '"' && left->text[strlen(left->text)-1] == '"') {
                            // String literal - remove quotes
                            size_t len = strlen(left->text);
                            char temp[256];
                            strncpy(temp, left->text + 1, len - 2);
                            temp[len - 2] = '\0';
                            strcat(result, temp);
                        } else {
                            // Variable reference
                            const char* str_val = get_string_variable(left->text);
                            if (str_val) {
                                strcat(result, str_val);
                            } else {
                                long long num_val = get_variable(left->text);
                                char num_str[64];
                                // Check if it's a float variable
                                if (is_float_variable(left->text)) {
                                    double float_val = (double)num_val / 1000000.0;
                                    snprintf(num_str, sizeof(num_str), "%.6g", float_val);
                                } else {
                                    snprintf(num_str, sizeof(num_str), "%lld", num_val);
                                }
                                strcat(result, num_str);
                            }
                        }
                    } else {
                        // Complex expression - evaluate it
                        long long num_val = eval_expression(left);
                        if (num_val == -1) {
                            // String result - get from last_concat_result
                            const char* concat_result = get_last_concat_result();
                            if (concat_result) {
                                strcat(result, concat_result);
                            }
                        } else {
                            char num_str[64];
                            snprintf(num_str, sizeof(num_str), "%lld", num_val);
                            strcat(result, num_str);
                        }
                    }
                    
                    // Right side
                    ASTNode* right = &expr->children[1];
                    if (right->text) {
                        if (right->text[0] == '"' && right->text[strlen(right->text)-1] == '"') {
                            // String literal - remove quotes
                            size_t len = strlen(right->text);
                            char temp[256];
                            strncpy(temp, right->text + 1, len - 2);
                            temp[len - 2] = '\0';
                            strcat(result, temp);
                                            } else if (right->child_count > 0 && (
                        strcmp(right->text, "+") == 0 || strcmp(right->text, "-") == 0 ||
                        strcmp(right->text, "*") == 0 || strcmp(right->text, "/") == 0 ||
                        strcmp(right->text, "%") == 0 || strcmp(right->text, "==") == 0 ||
                        strcmp(right->text, "!=") == 0 || strcmp(right->text, "<") == 0 ||
                        strcmp(right->text, ">") == 0 || strcmp(right->text, "<=") == 0 ||
                        strcmp(right->text, ">=") == 0)) {
                        // Complex expression with operator - evaluate it
                        long long num_val = eval_expression(right);
                        if (num_val == -1) {
                            // String result - get from last_concat_result
                            const char* concat_result = get_last_concat_result();
                            if (concat_result) {
                                strcat(result, concat_result);
                            }
                        } else {
                            char num_str[64];
                            snprintf(num_str, sizeof(num_str), "%lld", num_val);
                            strcat(result, num_str);
                        }
                    } else if (right->child_count > 0) {
                        // Complex expression (like function calls) - evaluate it
                        long long num_val = eval_expression(right);
                        if (num_val == -1) {
                            // String result - get from last_concat_result
                            const char* concat_result = get_last_concat_result();
                            if (concat_result) {
                                strcat(result, concat_result);
                            }
                        } else {
                            char num_str[64];
                            snprintf(num_str, sizeof(num_str), "%lld", num_val);
                            strcat(result, num_str);
                        }
                    } else {
                        // Variable reference or simple number
                        const char* str_val = get_string_variable(right->text);
                        if (str_val) {
                            strcat(result, str_val);
                        } else {
                            // Check if it's a simple number literal
                            char* endptr;
                            long long num_val = strtoll(right->text, &endptr, 10);
                            if (*endptr == '\0') {
                                // It's a valid number - convert to string
                                char num_str[64];
                                snprintf(num_str, sizeof(num_str), "%lld", num_val);
                                strcat(result, num_str);
                            } else {
                                // Try to get as variable
                                num_val = get_variable(right->text);
                                char num_str[64];
                                // Check if it's a float variable
                                if (is_float_variable(right->text)) {
                                    double float_val = (double)num_val / 1000000.0;
                                    snprintf(num_str, sizeof(num_str), "%.6g", float_val);
                                } else {
                                    snprintf(num_str, sizeof(num_str), "%lld", num_val);
                                }
                                strcat(result, num_str);
                            }
                        }
                    }
                    } else {
                        // Complex expression - evaluate it
                        printf("DEBUG: Evaluating right operand as complex expression\n");
                        long long num_val = eval_expression(right);
                        printf("DEBUG: Right operand evaluation result: %lld\n", num_val);
                        if (num_val == -1) {
                            // String result - get from last_concat_result
                            const char* concat_result = get_last_concat_result();
                            if (concat_result) {
                                strcat(result, concat_result);
                            }
                        } else {
                            char num_str[64];
                            snprintf(num_str, sizeof(num_str), "%lld", num_val);
                            strcat(result, num_str);
                        }
                    }
                    
                    printf("%s\n", result);
                } else if (expr->text) {
                    // Simple expression
                    if (expr->text[0] == '"' && expr->text[strlen(expr->text)-1] == '"') {
                        // String literal - remove quotes
                        size_t len = strlen(expr->text);
                        char* unquoted = malloc(len - 1);
                        strncpy(unquoted, expr->text + 1, len - 2);
                        unquoted[len - 2] = '\0';
                        printf("%s\n", unquoted);
                        free(unquoted);
                    } else {
                        // Variable reference
                        const char* str_val = get_string_variable(expr->text);
                        if (str_val) {
                            printf("%s\n", str_val);
                        } else {
                            long long num_val = get_variable(expr->text);
                            // Check if it's a float variable
                            if (is_float_variable(expr->text)) {
                                double float_val = (double)num_val / 1000000.0;
                                printf("%.6g\n", float_val);
                            } else {
                                printf("%lld\n", num_val);
                            }
                        }
                    }
                } else if (expr->child_count > 0) {
                    // Complex expression - evaluate it
                    long long num_val = eval_expression(expr);
                    if (num_val == -1) {
                        // String result - get from last_concat_result
                        const char* concat_result = get_last_concat_result();
                        if (concat_result) {
                            printf("%s\n", concat_result);
                        }
                    } else {
                        // Check if it's a float result
                        if (get_last_result_is_float()) {
                            double float_val = (double)num_val / 1000000.0;
                            printf("%.6g\n", float_val);
                        } else {
                            printf("%lld\n", num_val);
                        }
                    }
                }
            }
            break;
        }
        case AST_LET: {
            // Handle variable declarations
            if (ast->child_count >= 2) {
                char* var_name = ast->children[0].text;
                if (var_name) {
                    // Evaluate the right-hand side
                    ASTNode* value_node = &ast->children[1];
                    if (value_node->child_count > 0) {
                        // Complex expression - evaluate it
                        long long result = eval_expression(value_node);
                        if (result == -1) {
                            // String result - get from last_concat_result
                            const char* concat_result = get_last_concat_result();
                            if (concat_result) {
                                set_string_variable(var_name, concat_result);
                                printf("String variable '%s' = \"%s\"\n", var_name, concat_result);
                            }
                        } else {
                            // Check if the result should be treated as a float
                            if (get_last_result_is_float()) {
                                // Store as float variable
                                double float_val = (double)result / 1000000.0;
                                set_float_variable(var_name, float_val);
                                printf("Variable '%s' = %.6f\n", var_name, float_val);
                            } else {
                                // Store as regular variable
                                set_variable(var_name, result);
                                printf("Variable '%s' = %lld\n", var_name, result);
                            }
                        }
                    } else if (value_node->text) {
                        // Simple literal
                        // Check if it's a string literal
                        if (value_node->text[0] == '"' && value_node->text[strlen(value_node->text)-1] == '"') {
                            // It's a string - remove quotes and store
                            size_t len = strlen(value_node->text);
                            char* unquoted = malloc(len - 1);
                            strncpy(unquoted, value_node->text + 1, len - 2);
                            unquoted[len - 2] = '\0';
                            set_string_variable(var_name, unquoted);
                            printf("String variable '%s' = \"%s\"\n", var_name, unquoted);
                            free(unquoted);
                        } else {
                            // Try to parse as number
                            char* endptr;
                            
                            // First check if it contains a decimal point (float)
                            if (strchr(value_node->text, '.') != NULL) {
                                // It's a float
                                double float_val = strtod(value_node->text, &endptr);
                                if (*endptr == '\0') {
                                    // Use the float variable function
                                    set_float_variable(var_name, float_val);
                                    printf("Variable '%s' = %.6f\n", var_name, float_val);
                                } else {
                                    printf("Variable '%s' declared (invalid float)\n", var_name);
                                }
                            } else {
                                // It's an integer
                                long long value = strtoll(value_node->text, &endptr, 10);
                                if (*endptr == '\0') {
                                    set_variable(var_name, value);
                                    printf("Variable '%s' = %lld\n", var_name, value);
                                } else {
                                    printf("Variable '%s' declared (unknown type)\n", var_name);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case AST_ASSIGN: {
            // Handle variable assignments (reassignments)
            if (ast->child_count >= 2) {
                char* var_name = ast->children[0].text;
                if (var_name) {
                    // Evaluate the right-hand side
                    ASTNode* value_node = &ast->children[1];
                    if (value_node->child_count > 0) {
                        // Complex expression - evaluate it
                        long long result = eval_expression(value_node);
                        if (result == -1) {
                            // String result - get from last_concat_result
                            const char* concat_result = get_last_concat_result();
                            if (concat_result) {
                                set_string_variable(var_name, concat_result);
                                printf("String variable '%s' = \"%s\"\n", var_name, concat_result);
                            }
                        } else {
                            // Check if the result should be treated as a float
                            if (get_last_result_is_float()) {
                                // Store as float variable
                                double float_val = (double)result / 1000000.0;
                                set_float_variable(var_name, float_val);
                                printf("Variable '%s' = %.6f\n", var_name, float_val);
                            } else {
                                // Store as regular variable
                                set_variable(var_name, result);
                                printf("Variable '%s' = %lld\n", var_name, result);
                            }
                        }
                    } else if (value_node->text) {
                        // Simple literal
                        // Check if it's a string literal
                        if (value_node->text[0] == '"' && value_node->text[strlen(value_node->text)-1] == '"') {
                            // It's a string - remove quotes and store
                            size_t len = strlen(value_node->text);
                            char* unquoted = malloc(len - 1);
                            strncpy(unquoted, value_node->text + 1, len - 2);
                            unquoted[len - 2] = '\0';
                            set_string_variable(var_name, unquoted);
                            printf("String variable '%s' = \"%s\"\n", var_name, unquoted);
                            free(unquoted);
                        } else {
                            // Try to parse as number
                            char* endptr;
                            
                            // First check if it contains a decimal point (float)
                            if (strchr(value_node->text, '.') != NULL) {
                                // It's a float
                                double float_val = strtod(value_node->text, &endptr);
                                if (*endptr == '\0') {
                                    // Use the float variable function
                                    set_float_variable(var_name, float_val);
                                    printf("Variable '%s' = %.6f\n", var_name, float_val);
                                } else {
                                    printf("Variable '%s' declared (invalid float)\n", var_name);
                                }
                            } else {
                                // It's an integer
                                long long value = strtoll(value_node->text, &endptr, 10);
                                if (*endptr == '\0') {
                                    set_variable(var_name, value);
                                    printf("Variable '%s' = %lld\n", var_name, value);
                                } else {
                                    printf("Variable '%s' declared (invalid type)\n", var_name);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case AST_FUNC: {
            // Handle function definitions
            if (ast->text) {
                register_function(ast->text, ast);
                printf("Function '%s' defined\n", ast->text);
            }
            break;
        }
        case AST_BLOCK: {
            // Handle blocks of statements
            for (int i = 0; i < ast->child_count; i++) {
                eval_evaluate(&ast->children[i]);
            }
            break;
        }
        default: {
            // Handle other node types
            for (int i = 0; i < ast->child_count; i++) {
                eval_evaluate(&ast->children[i]);
            }
            break;
        }
    }
}

void cleanup_libraries(void) {
    // TODO: Cleanup libraries
}

void cleanup_implicit_functions(void) {
    // TODO: Cleanup implicit functions
}

void cleanup_loop_execution_state(void) {
    // TODO: Cleanup loop execution state
}

/*******************************************************************************
 * MAIN ENTRY POINT
 ******************************************************************************/

/**
 * @brief Main entry point for the Myco interpreter
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return Exit code (0 for success, non-zero for errors)
 * 
 * This function orchestrates the complete interpretation process:
 * - Parses command line arguments
 * - Loads and validates source files
 * - Coordinates lexical analysis, parsing, and execution
 * - Handles errors and provides cleanup
 * - Supports both interpretation and build modes
 */
int main(int argc, char* argv[]) {
    #if DEBUG_MEMORY_TRACKING
    memory_tracker_init();
    #endif
    
    // Make prompts visible immediately in interactive mode
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [--build] [--output]\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    int build_mode = 0;
    const char* output_file = NULL;

    /*******************************************************************************
     * COMMAND LINE ARGUMENT PARSING
     ******************************************************************************/
    
    // Parse command line arguments for build mode and output file specification
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--build") == 0) {
            build_mode = 1;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    /*******************************************************************************
     * SOURCE FILE LOADING AND VALIDATION
     ******************************************************************************/
    
    // Open and read input file with error handling
    FILE* file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", input_file);
        return 1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer and read file
    char* source_code = malloc(file_size + 1);
    if (!source_code) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    size_t bytes_read = fread(source_code, 1, file_size, file);
    source_code[bytes_read] = '\0';
    fclose(file);
    
    #if DEBUG_MEMORY_TRACKING
    if (build_mode) {
        printf("Building executable from %s...\n", input_file);
    } else {
        printf("Interpreting %s...\n", input_file);
    }
    #endif
    
    /*******************************************************************************
     * INTERPRETATION PIPELINE
     ******************************************************************************/
    
    // Phase 1: Lexical Analysis - Convert source code to tokens
    Token* tokens = lexer_tokenize(source_code);
    if (!tokens) {
        fprintf(stderr, "Error: Lexical analysis failed\n");
        free(source_code);
        return 1;
    }
    
    /*******************************************************************************
     * MODULE IMPORT PATH RESOLUTION
     ******************************************************************************/
    
    // Set base directory for resolving relative module imports
    {
        // derive directory part from input_file
        const char* last_slash = strrchr(input_file, '/');
        char dirbuf[1024];
        if (last_slash) {
            size_t n = (size_t)(last_slash - input_file);
            if (n >= sizeof(dirbuf)) n = sizeof(dirbuf) - 1;
            memcpy(dirbuf, input_file, n);
            dirbuf[n] = '\0';
        } else {
            strcpy(dirbuf, ".");
        }
        eval_set_base_dir(dirbuf);
    }

    // Initialize environments for clean execution
    // (Don't call eval_reset_environments here as it tries to cleanup uninitialized environments)

    // Phase 2: Parsing - Convert tokens to Abstract Syntax Tree (AST)
    ASTNode* ast = parser_parse(tokens);
    if (!ast) {
        fprintf(stderr, "Error: Parsing failed\n");
        free(tokens);
        free(source_code);
        return 1;
    }
    
    if (build_mode) {
        // Code generation mode
        const char* output_name = output_file ? output_file : "output.c";
        if (codegen_generate(ast, output_name, 0) == 0) {
            printf("Executable generated successfully.\n");
        } else {
            fprintf(stderr, "Error: Code generation failed\n");
        }
    } else {
        // Initialize implicit function system
        init_implicit_functions();
        
        // Initialize library system
        init_libraries();
        
        // Set command-line arguments for the args library
        set_command_line_args(argc, argv);
        
        // Evaluate the AST
        eval_evaluate(ast);
        
        // Cleanup library system
        cleanup_libraries();
    }
    
    // Cleanup
    parser_free_ast(ast);
    free(tokens);
    free(source_code);
    
    #if DEBUG_MEMORY_TRACKING
    cleanup_all_environments();
    memory_tracker_cleanup();
    #endif
    
    // Cleanup implicit function system
    cleanup_implicit_functions();
    
    // Cleanup loop execution state
    extern void cleanup_loop_execution_state(void);
    cleanup_loop_execution_state();
    
    return 0;
} 