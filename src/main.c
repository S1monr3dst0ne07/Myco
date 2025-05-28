#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s [--build] <input_file>\n", program_name);
    fprintf(stderr, "  --build: Generate C code and compile to executable\n");
    fprintf(stderr, "  <input_file>: Path to the Myco source file\n");
}

// Helper to read file into a string
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = malloc(size + 1);
    if (!buffer) { fclose(file); return NULL; }
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* input_file = NULL;
    bool build_mode = false;

    if (argc == 3) {
        if (strcmp(argv[1], "--build") == 0) {
            build_mode = true;
            input_file = argv[2];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    } else {
        input_file = argv[1];
    }

    char* source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "Error: Failed to read input file\n");
        return 1;
    }

    Lexer* lexer = lexer_init(source);
    if (!lexer) {
        fprintf(stderr, "Error: Failed to initialize lexer\n");
        free(source);
        return 1;
    }

    Parser* parser = parser_init(lexer);
    if (!parser) {
        fprintf(stderr, "Error: Failed to initialize parser\n");
        lexer_free(lexer);
        free(source);
        return 1;
    }

    ASTNode* ast = parser_parse(parser);
    if (!ast) {
        fprintf(stderr, "Error: Failed to parse input\n");
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    if (build_mode) {
        CodeGenerator* codegen = codegen_init("output.c");
        if (!codegen) {
            fprintf(stderr, "Error: Failed to initialize code generator\n");
            ast_node_free(ast);
            parser_free(parser);
            lexer_free(lexer);
            free(source);
            return 1;
        }
        if (!codegen_generate(codegen, ast)) {
            fprintf(stderr, "Error: Failed to generate C code\n");
            codegen_free(codegen);
            ast_node_free(ast);
            parser_free(parser);
            lexer_free(lexer);
            free(source);
            return 1;
        }
        codegen_free(codegen);
    } else {
        interpret_ast(ast);
    }

    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    return 0;
} 