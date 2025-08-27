#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_FUNC, AST_LET, AST_IF, AST_FOR, AST_WHILE, AST_RETURN,
    AST_SWITCH, AST_CASE, AST_DEFAULT, AST_TRY, AST_CATCH, AST_PRINT,
    AST_EXPR, AST_BLOCK, AST_DOT, AST_ASSIGN, AST_ARRAY_LITERAL,
    AST_ARRAY_ACCESS, AST_ARRAY_ASSIGN, AST_OBJECT_LITERAL,
    AST_OBJECT_ACCESS, AST_OBJECT_ASSIGN, AST_OBJECT_BRACKET_ACCESS,
    AST_OBJECT_BRACKET_ASSIGN, AST_LAMBDA, AST_TERNARY
} ASTNodeType;

// Enhanced for loop types
typedef enum {
    AST_FOR_RANGE,        // for i in 1..10:
    AST_FOR_ARRAY,        // for item in array:
    AST_FOR_STEP,         // for i in 1:10:2:
    AST_FOR_DOWNTO        // for i in 10:1:-1:
} ForLoopType;

typedef struct ASTNode {
    ASTNodeType type;
    char* text;  // For identifiers, numbers, strings, etc.
    char* implicit_function;  // NEW: stores function name for implicit calls
    struct ASTNode* children;  // Array of child nodes
    int child_count;
    struct ASTNode* next;  // For linked list of statements
    int line;  // Add line number field
    
    // Enhanced for loop support
    ForLoopType for_type; // Specific for loop variant (only used when type == AST_FOR)
} ASTNode;

// Function prototypes
ASTNode* parser_parse(Token* tokens);
void parser_free_ast(ASTNode* ast);

#endif // PARSER_H 