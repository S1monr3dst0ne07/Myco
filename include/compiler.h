#ifndef MYCO_COMPILER_H
#define MYCO_COMPILER_H

#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <stdbool.h>

typedef struct {
    const char* source_file;
    const char* output_file;
    bool build_executable;
    bool verbose;
} CompilerOptions;

typedef struct {
    CompilerOptions options;
    Lexer* lexer;
    Parser* parser;
    CodeGenerator* codegen;
    bool had_error;
} Compiler;

// Compiler functions
Compiler* compiler_init(CompilerOptions options);
void compiler_free(Compiler* compiler);
bool compiler_compile(Compiler* compiler);
bool compiler_compile_file(const char* input_file, const char* output_file, bool build_executable);

// Error handling
void compiler_error(Compiler* compiler, const char* format, ...);
void compiler_warning(Compiler* compiler, const char* format, ...);

#endif // MYCO_COMPILER_H 