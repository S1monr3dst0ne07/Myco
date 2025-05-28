#include "../include/compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

Compiler* compiler_init(CompilerOptions options) {
    Compiler* compiler = malloc(sizeof(Compiler));
    if (!compiler) return NULL;
    
    compiler->options = options;
    compiler->had_error = false;
    
    // Read source file
    FILE* file = fopen(options.source_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", options.source_file);
        free(compiler);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file content
    char* source = malloc(size + 1);
    if (!source) {
        fclose(file);
        free(compiler);
        return NULL;
    }
    
    size_t read = fread(source, 1, size, file);
    source[read] = '\0';
    fclose(file);
    
    // Initialize components
    compiler->lexer = lexer_init(source);
    if (!compiler->lexer) {
        free(source);
        free(compiler);
        return NULL;
    }
    
    compiler->parser = parser_init(compiler->lexer);
    if (!compiler->parser) {
        lexer_free(compiler->lexer);
        free(source);
        free(compiler);
        return NULL;
    }
    
    compiler->codegen = codegen_init(options.output_file);
    if (!compiler->codegen) {
        parser_free(compiler->parser);
        lexer_free(compiler->lexer);
        free(source);
        free(compiler);
        return NULL;
    }
    
    return compiler;
}

void compiler_free(Compiler* compiler) {
    if (compiler->codegen) codegen_free(compiler->codegen);
    if (compiler->parser) parser_free(compiler->parser);
    if (compiler->lexer) lexer_free(compiler->lexer);
    free(compiler);
}

bool compiler_compile(Compiler* compiler) {
    if (compiler->options.verbose) {
        printf("Compiling %s...\n", compiler->options.source_file);
    }
    
    // Parse the source code
    ASTNode* ast = parser_parse(compiler->parser);
    if (!ast || compiler->parser->had_error) {
        fprintf(stderr, "Error: Parsing failed\n");
        return false;
    }
    
    // Generate C code
    if (!codegen_generate(compiler->codegen, ast)) {
        fprintf(stderr, "Error: Code generation failed\n");
        ast_node_free(ast);
        return false;
    }
    
    ast_node_free(ast);
    
    // If building executable, compile the generated C code
    if (compiler->options.build_executable) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "gcc -O2 -o %s %s",
                compiler->options.output_file,
                compiler->options.output_file);
        
        if (system(cmd) != 0) {
            fprintf(stderr, "Error: Failed to compile generated C code\n");
            return false;
        }
        
        // Remove the intermediate .c file
        remove(compiler->options.output_file);
    }
    
    if (compiler->options.verbose) {
        printf("Compilation successful\n");
    }
    
    return true;
}

bool compiler_compile_file(const char* input_file, const char* output_file, bool build_executable) {
    CompilerOptions options = {
        .source_file = input_file,
        .output_file = output_file,
        .build_executable = build_executable,
        .verbose = true
    };
    
    Compiler* compiler = compiler_init(options);
    if (!compiler) return false;
    
    bool success = compiler_compile(compiler);
    compiler_free(compiler);
    
    return success;
}

void compiler_error(Compiler* compiler, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    compiler->had_error = true;
}

void compiler_warning(Compiler* compiler, const char* format, ...) {
    if (!compiler->options.verbose) return;
    
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
} 