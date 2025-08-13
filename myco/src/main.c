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

    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--build") == 0) {
            build_mode = 1;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    // Open and read input file
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
    
    // Lexical analysis
    Token* tokens = lexer_tokenize(source_code);
    if (!tokens) {
        fprintf(stderr, "Error: Lexical analysis failed\n");
        free(source_code);
        return 1;
    }
    
    // Set base directory for imports
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

    // Parsing
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
        // Evaluate the AST
            eval_evaluate(ast);
    }
    
    // Cleanup
    parser_free_ast(ast);
    free(tokens);
    free(source_code);
    
    #if DEBUG_MEMORY_TRACKING
    cleanup_all_environments();
    memory_tracker_cleanup();
    #endif
    
    // Cleanup loop execution state
    extern void cleanup_loop_execution_state(void);
    cleanup_loop_execution_state();
    
    return 0;
} 