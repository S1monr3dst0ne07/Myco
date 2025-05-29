#ifndef MYCO_CODEGEN_H
#define MYCO_CODEGEN_H

#include "parser.h"
#include <stdbool.h>

typedef struct {
    FILE* output;
    int indent_level;
    bool had_error;
} CodeGenerator;

// Code generator functions
CodeGenerator* codegen_init(const char* output_file);
void codegen_free(CodeGenerator* gen);
bool codegen_generate(CodeGenerator* gen, ASTNode* ast);
void codegen_generate_header(CodeGenerator* gen);
void codegen_generate_footer(CodeGenerator* gen);

// Helper functions for code generation
void codegen_indent(CodeGenerator* gen);
void codegen_dedent(CodeGenerator* gen);
void codegen_write(CodeGenerator* gen, const char* format, ...);
void codegen_write_line(CodeGenerator* gen, const char* format, ...);

#endif // MYCO_CODEGEN_H 