#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"

// Function prototypes
int codegen_generate(ASTNode* ast, const char* filename, int keep_output);

#endif // CODEGEN_H 