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
    char* source_code = tracked_malloc(file_size + 1, __FILE__, __LINE__, "main_source_code");
    if (!source_code) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    size_t bytes_read = fread(source_code, 1, file_size, file);
    source_code[bytes_read] = '\0';
    fclose(file);
    
    if (build_mode) {
        printf("Building executable from %s...\n", input_file);
    }
    
    /*******************************************************************************
     * INTERPRETATION PIPELINE
     ******************************************************************************/
    
    // Phase 1: Lexical Analysis - Convert source code to tokens
    Token* tokens = lexer_tokenize(source_code);
    if (!tokens) {
        fprintf(stderr, "Error: Lexical analysis failed\n");
        tracked_free(source_code, __FILE__, __LINE__, "main_lexical_error");
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
        tracked_free(tokens, __FILE__, __LINE__, "main_parsing_error");
        tracked_free(source_code, __FILE__, __LINE__, "main_parsing_error");
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
    tracked_free(tokens, __FILE__, __LINE__, "main_cleanup");
    tracked_free(source_code, __FILE__, __LINE__, "main_cleanup");
    
    #if DEBUG_MEMORY_TRACKING
    cleanup_all_environments();
    #endif
    
    // Cleanup implicit function system
    cleanup_implicit_functions();
    
    // Cleanup loop execution state
    extern void cleanup_loop_execution_state(void);
    cleanup_loop_execution_state();
    
    #if DEBUG_MEMORY_TRACKING
    // Memory tracker cleanup must be LAST after all other cleanup functions
    memory_tracker_cleanup();
    #endif
    
    return 0;
} 