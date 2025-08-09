#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_FUNC, AST_LET, AST_IF, AST_FOR, AST_WHILE, AST_RETURN,
    AST_SWITCH, AST_CASE, AST_DEFAULT, AST_TRY, AST_CATCH, AST_PRINT,
    AST_EXPR, AST_BLOCK
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char* text;  // For identifiers, numbers, strings, etc.
    struct ASTNode* children;  // Array of child nodes
    int child_count;
    struct ASTNode* next;  // For linked list of statements
    int line;  // Add line number field
} ASTNode;

// Function prototypes
ASTNode* parser_parse(Token* tokens);
void parser_free_ast(ASTNode* ast);

#endif // PARSER_H 