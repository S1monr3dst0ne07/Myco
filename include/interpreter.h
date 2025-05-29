#ifndef MYCO_INTERPRETER_H
#define MYCO_INTERPRETER_H

#include "parser.h"

// Function to interpret the entire AST
int64_t interpret_ast(ASTNode* ast);

int64_t interpret_node(ASTNode* node);

#endif // MYCO_INTERPRETER_H 