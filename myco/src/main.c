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

// Forward declaration for debug mode function
extern void set_debug_mode(int enabled);

/*******************************************************************************
 * HELP AND USAGE FUNCTIONS
 ******************************************************************************/

/**
 * @brief Display comprehensive help information
 * @param program_name Name of the program executable
 */
void print_help(const char* program_name) {
    printf("\n");
    printf("🌱 MYCO PROGRAMMING LANGUAGE INTERPRETER v1.6.0\n");
    printf("================================================\n\n");
    
    printf("USAGE:\n");
    printf("  %s <input_file> [options]\n", program_name);
    printf("  %s --help\n", program_name);
    printf("  %s --version\n", program_name);
    printf("\n");
    
    printf("ARGUMENTS:\n");
    printf("  <input_file>    Myco source file (.myco) to interpret or compile\n");
    printf("\n");
    
    printf("OPTIONS:\n");
    printf("  --help          Show this help message and exit\n");
    printf("  --version       Show version information and exit\n");
    printf("  --debug         Enable debug mode with colored initialization messages\n");
    printf("  --build         Generate C output instead of interpreting\n");
    printf("  --output <file> Specify output file for build mode\n");
    printf("  --optimize      Enable performance optimizations (default: enabled)\n");
    printf("  --no-optimize   Disable performance optimizations\n");
    printf("  --verbose       Show detailed execution information\n");
    printf("  --quiet         Suppress non-essential output\n");
    printf("\n");
    
    printf("BUILD MODE:\n");
    printf("  --build         Generate C source code output\n");
    printf("  --output <file> Write C output to specified file (default: output.c)\n");
    printf("  --compile       Compile generated C code to executable\n");
    printf("  --optimize-c    Enable C compiler optimizations\n");
    printf("\n");
    
    printf("DEBUGGING:\n");
    printf("  --debug         Show colored initialization and cleanup messages\n");
    printf("  --trace         Enable execution tracing\n");
    printf("  --profile       Enable performance profiling\n");
    printf("  --memory        Show memory allocation statistics\n");
    printf("\n");
    
    printf("EXAMPLES:\n");
    printf("  %s program.myco                    # Interpret Myco program\n", program_name);
    printf("  %s program.myco --debug            # Run with debug output\n", program_name);
    printf("  %s program.myco --build            # Generate C output\n", program_name);
    printf("  %s program.myco --build --output my_program.c\n", program_name);
    printf("  %s --help                          # Show this help\n", program_name);
    printf("\n");
    
    printf("BUILDING FROM SOURCE:\n");
    printf("  git clone https://github.com/IvyMycelia/myco.git\n");
    printf("  cd myco/myco\n");
    printf("  make                    # Development build with debug info\n");
    printf("  make release            # Optimized release build\n");
    printf("  make prod              # Maximum optimization build\n");
    printf("  make pgo               # Profile-guided optimization build\n");
    printf("  make windows           # Cross-compile for Windows\n");
    printf("  make arm64             # Apple Silicon optimized build\n");
    printf("\n");
    
    printf("FEATURES:\n");
    printf("  • Dynamic typing with clear type names\n");
    printf("  • Object-oriented programming with nested objects\n");
    printf("  • Functional programming with lambda functions\n");
    printf("  • Comprehensive standard library\n");
    printf("  • Cross-platform compatibility (Windows, macOS, Linux)\n");
    printf("  • High-performance execution with optimizations\n");
    printf("  • Memory-safe execution with tracking\n");
    printf("  • Professional testing framework\n");
    printf("\n");
    
    printf("DOCUMENTATION:\n");
    printf("  • Language Reference: Documentation.md\n");
    printf("  • Grammar Specification: BNF_Grammar.md\n");
    printf("  • Development Roadmap: DevelopmentPlan.md\n");
    printf("  • Repository: https://github.com/IvyMycelia/myco\n");
    printf("\n");
    
    printf("LICENSE: MIT License - Open source and free to use\n");
    printf("VERSION: 1.6.0 - Language Maturity & Developer Experience\n");
    printf("\n");
}

/**
 * @brief Display version information
 */
void print_version() {
    printf("Myco Programming Language Interpreter v1.6.0\n");
    printf("Language Maturity & Developer Experience\n");
    printf("MIT License - https://github.com/IvyMycelia/myco\n");
    printf("Cross-platform: Windows, macOS, Linux\n");
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
    // Check for help and version flags first
    if (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--version") == 0) {
            print_version();
            return 0;
        }
    }
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [options] or %s --help for more information\n", argv[0], argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    int build_mode = 0;
    int debug_mode = 0;
    int verbose_mode = 0;
    int quiet_mode = 0;
    int optimize_mode = 1;  // Default: enabled
    const char* output_file = NULL;

    /*******************************************************************************
     * COMMAND LINE ARGUMENT PARSING
     ******************************************************************************/
    
    // Parse command line arguments for all supported flags
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--build") == 0) {
            build_mode = 1;
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose_mode = 1;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet_mode = 1;
        } else if (strcmp(argv[i], "--optimize") == 0) {
            optimize_mode = 1;
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            optimize_mode = 0;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else {
            fprintf(stderr, "Warning: Unknown option '%s'. Use --help for available options.\n", argv[i]);
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
        if (verbose_mode) {
            printf("🌱 Building executable from %s...\n", input_file);
            printf("📁 Input file: %s\n", input_file);
            if (output_file) {
                printf("📄 Output file: %s\n", output_file);
            } else {
                printf("📄 Output file: output.c (default)\n");
            }
            printf("⚡ Optimization: %s\n", optimize_mode ? "enabled" : "disabled");
        } else {
            printf("Building executable from %s...\n", input_file);
        }
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
        // Set debug mode if requested
        set_debug_mode(debug_mode);
        
        // Set debug mode for memory tracker if requested
        #if DEBUG_MEMORY_TRACKING
        extern void memory_tracker_set_debug_mode(int enabled);
        memory_tracker_set_debug_mode(debug_mode);
        #endif
        
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
    
    // PHASE 2: Cleanup targeted bottleneck optimization systems
    extern void cleanup_phase2_optimization_systems(void);
    cleanup_phase2_optimization_systems();
    
    // Cleanup loop execution state
    extern void cleanup_loop_execution_state(void);
    cleanup_loop_execution_state();
    
    #if DEBUG_MEMORY_TRACKING
    // Memory tracker cleanup must be LAST after all other cleanup functions
    memory_tracker_cleanup();
    #endif
    
    return 0;
} 