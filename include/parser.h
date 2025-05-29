#ifndef MYCO_PARSER_H
#define MYCO_PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DEF,
    NODE_VAR_DECL,
    NODE_ASSIGNMENT,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_IF_STMT,
    NODE_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_SWITCH_STMT,
    NODE_TRY_STMT,
    NODE_FUNCTION_CALL,
    NODE_RETURN_STMT,
    NODE_BLOCK,
    NODE_TYPE_ANNOTATION
} NodeType;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode* next;  // For linked list of statements
    union {
        // Literal values
        struct {
            TokenType value_type;
            union {
                int64_t int_value;
                double float_value;
                char* string_value;
                bool bool_value;
            };
        } literal;
        
        // Binary operation
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
            TokenType operator;
        } binary;
        
        // Unary operation
        struct {
            struct ASTNode* operand;
            TokenType operator;
        } unary;
        
        // Variable declaration
        struct {
            char* name;
            struct ASTNode* initializer;
            struct ASTNode* type_annotation;
            bool is_const;
        } var_decl;
        
        // Function definition
        struct {
            char* name;
            struct ASTNode* params;
            struct ASTNode* return_type;
            struct ASTNode* body;
        } function;
        
        // If statement
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } if_stmt;
        
        // While statement
        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_stmt;
        
        // For statement
        struct {
            char* var_name;
            struct ASTNode* start;
            struct ASTNode* end;
            struct ASTNode* body;
        } for_stmt;
        
        // Switch statement
        struct {
            struct ASTNode* value;
            struct ASTNode* case_value;
            struct ASTNode* case_body;
            struct ASTNode* default_body;
        } switch_stmt;
        
        // Try statement
        struct {
            struct ASTNode* try_body;
            struct ASTNode* catch_body;
        } try_stmt;
        
        // Block of statements
        struct {
            struct ASTNode* statements;
        } block;
    } as;
} ASTNode;

typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    bool had_error;
} Parser;

// Parser functions
Parser* parser_init(Lexer* lexer);
void parser_free(Parser* parser);
ASTNode* parser_parse(Parser* parser);
void ast_node_free(ASTNode* node);

#endif // MYCO_PARSER_H 