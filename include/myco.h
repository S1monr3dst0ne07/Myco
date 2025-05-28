#ifndef MYCO_H
#define MYCO_H

#include <stdbool.h>
#include <stddef.h>

#define MYCO_MAX_TOKEN_LEN 256
#define MYCO_MAX_ERROR_LENGTH 256

// Token types
typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_COLON, TOKEN_PERCENT, TOKEN_BANG,
    TOKEN_ASSIGN, // =
    
    // One or two character tokens
    TOKEN_BANG_EQUAL, TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL, TOKEN_GREATER,
    TOKEN_GREATER_EQUAL, TOKEN_LESS,
    TOKEN_LESS_EQUAL, TOKEN_DOT_DOT,
    TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS,
    TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN, TOKEN_STAR_ASSIGN, TOKEN_SLASH_ASSIGN,
    
    // Additional tokens for codegen
    TOKEN_EQ,        // ==
    TOKEN_NEQ,       // !=
    TOKEN_LT,        // <
    TOKEN_GT,        // >
    TOKEN_LTE,       // <=
    TOKEN_GTE,       // >=
    TOKEN_DOTDOT,    // ..
    TOKEN_NOT,       // not
    TOKEN_STRING_TYPE, // string type
    
    // Literals
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    
    // Keywords
    TOKEN_AND, TOKEN_BREAK, TOKEN_CASE, TOKEN_CATCH,
    TOKEN_CONST, TOKEN_CONTINUE, TOKEN_DEFAULT,
    TOKEN_ELSE, TOKEN_ELSEIF, TOKEN_END, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUNC, TOKEN_IF, TOKEN_IN,
    TOKEN_LET, TOKEN_NONE, TOKEN_OR, TOKEN_PRINT,
    TOKEN_RETURN, TOKEN_SWITCH, TOKEN_TRY, TOKEN_TRUE,
    TOKEN_VAR, TOKEN_WHILE,
    TOKEN_IMPORT, TOKEN_ASYNC,
    
    // Built-in types
    TOKEN_INT, TOKEN_FLOAT, TOKEN_STR, TOKEN_BOOL, TOKEN_LIST, TOKEN_MAP,
    
    TOKEN_ERROR, TOKEN_EOF
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    int col;
} Token;

// Lexer structure
typedef struct {
    const char* source;
    size_t length;
    size_t pos;
    int line;
    int col;
} Lexer;

// AST node types
typedef enum {
    NODE_BLOCK,
    NODE_LITERAL,
    NODE_VARIABLE,
    NODE_ASSIGNMENT,
    NODE_BINARY,
    NODE_UNARY,
    NODE_PRINT,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_SWITCH,
    NODE_CASE,
    NODE_TRY,
    NODE_CATCH,
    NODE_FUNCTION,
    NODE_CALL,
    NODE_RETURN,
    NODE_VAR_DECL,
    NODE_CONST_DECL,
    NODE_LIST,
    NODE_MAP,
    NODE_NONE
} NodeType;

// AST node structure
typedef struct Node {
    NodeType type;
    struct Node* left;
    struct Node* right;
    double number; // For NODE_LITERAL
    const char* string; // For NODE_LITERAL
    union {
        Token identifier;
        struct {
            struct Node* params;
            struct Node* body;
        } function;
        struct {
            struct Node* callee;
            struct Node* args;
        } call;
    } value;
} Node;

// Function declarations
void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);
Node* parse(const char* source);
void interpret(Node* node);
void free_ast(Node* node);

#endif // MYCO_H 