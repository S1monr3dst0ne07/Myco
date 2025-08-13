#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "codegen.h"

int main(int argc, char* argv[]) {
    // Make prompts visible immediately in interactive mode
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [--build] [--output]\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    int build_mode = 0;
    int keep_output = 0;

    // Parse command-line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--build") == 0) {
            build_mode = 1;
        } else if (strcmp(argv[i], "--output") == 0) {
            keep_output = 1;
        }
    }

    // Open the input file
    FILE* file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", input_file);
        return 1;
    }

    // Read the file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* source = malloc(file_size + 1);
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    // Lexical analysis
    Token* tokens = lexer_tokenize(source);
    if (!tokens) {
        fprintf(stderr, "Error: Lexical analysis failed\n");
        free(source);
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
        free(source);
        return 1;
    }

    if (build_mode) {
        // Generate executable
        if (codegen_generate(ast, input_file, keep_output) != 0) {
            fprintf(stderr, "Error: Code generation failed\n");
            free(source);
            return 1;
        }
        printf("Executable generated successfully.\n");
    } else {
        // Evaluate the AST
            eval_evaluate(ast);
    }

    // Cleanup
    free(source);
    lexer_free_tokens(tokens);
    
    if (!build_mode) {
        // Clear module AST references before freeing main AST to prevent double-free
        extern void eval_clear_module_asts();
        extern void eval_clear_function_asts();
        eval_clear_module_asts();
        eval_clear_function_asts();
        
        // Don't call eval_reset_environments() as it causes double-free issues
        // The environments will be cleaned up when the program exits
    }
    
    parser_free_ast(ast);
    
    return 0;
} 