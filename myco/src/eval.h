#ifndef EVAL_H
#define EVAL_H

#include "parser.h"

// Function declarations
void eval_set_base_dir(const char* dir);
void eval_evaluate(struct ASTNode* ast);
long long eval_expression(struct ASTNode* ast);
void eval_reset_environments(void);

// Error handling
extern int error_occurred;
extern int error_value;
extern int current_line;

#endif // EVAL_H
