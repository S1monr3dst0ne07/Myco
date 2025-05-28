#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

typedef struct {
    FILE *output;
    int indent_level;
    bool in_function;
    const char *current_function;
} CodeGen;

void codegen_init(CodeGen *gen, FILE *output);
void codegen_generate(CodeGen *gen, ASTNode *ast);
void codegen_free(CodeGen *gen);

#endif // CODEGEN_H 