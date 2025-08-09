#ifndef EVAL_H
#define EVAL_H

#include "parser.h"

// Function prototypes
void eval_evaluate(ASTNode* ast);
void eval_set_base_dir(const char* dir);

#endif // EVAL_H 