#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR,
    TOKEN_ASSIGN,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COMMA,
    TOKEN_FUNC,
    TOKEN_LET,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_END,
    TOKEN_RETURN,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_TRY,
    TOKEN_CATCH,
    TOKEN_PRINT,
    TOKEN_TYPE_MARKER,
    TOKEN_STRING_TYPE,
    TOKEN_IN,
    TOKEN_USE,
    TOKEN_AS,
    TOKEN_PATH,
    TOKEN_DOT,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE
} MycoTokenType;

typedef struct {
    MycoTokenType type;
    char* text;
    int line;
} Token;

// Function prototypes
Token* lexer_tokenize(const char* source);
void lexer_free_tokens(Token* tokens);

#endif // LEXER_H 