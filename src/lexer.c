#include <stdbool.h>
#include "../include/lexer.h"
#include <ctype.h>
#include <string.h>

static const char* token_type_strings[] = {
    "let", "var", "const", "func", "return", "if", "else", "elseif", "end",
    "while", "for", "in", "switch", "case", "default", "try", "catch",
    "int", "float", "string", "bool",
    "integer", "float_literal", "string_literal", "true", "false", "null",
    "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=",
    "and", "or", "not", "..",
    "(", ")", "{", "}", "[", "]", ":", ";", ",", ".", "->",
    "identifier", "EOF", "error"
};

static bool is_keyword(const char* str) {
    const char* keywords[] = {
        "let", "var", "const", "func", "return", "if", "else", "elseif", "end",
        "while", "for", "in", "switch", "case", "default", "try", "catch",
        "int", "float", "string", "bool", "true", "false", "null"
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(str, keywords[i]) == 0) return true;
    }
    return false;
}

static TokenType keyword_to_token(const char* str) {
    if (strcmp(str, "let") == 0) return TOKEN_LET;
    if (strcmp(str, "var") == 0) return TOKEN_VAR;
    if (strcmp(str, "const") == 0) return TOKEN_CONST;
    if (strcmp(str, "func") == 0) return TOKEN_FUNC;
    if (strcmp(str, "return") == 0) return TOKEN_RETURN;
    if (strcmp(str, "if") == 0) return TOKEN_IF;
    if (strcmp(str, "else") == 0) return TOKEN_ELSE;
    if (strcmp(str, "elseif") == 0) return TOKEN_ELSEIF;
    if (strcmp(str, "end") == 0) return TOKEN_END;
    if (strcmp(str, "while") == 0) return TOKEN_WHILE;
    if (strcmp(str, "for") == 0) return TOKEN_FOR;
    if (strcmp(str, "in") == 0) return TOKEN_IN;
    if (strcmp(str, "switch") == 0) return TOKEN_SWITCH;
    if (strcmp(str, "case") == 0) return TOKEN_CASE;
    if (strcmp(str, "default") == 0) return TOKEN_DEFAULT;
    if (strcmp(str, "try") == 0) return TOKEN_TRY;
    if (strcmp(str, "catch") == 0) return TOKEN_CATCH;
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    if (strcmp(str, "float") == 0) return TOKEN_FLOAT;
    if (strcmp(str, "string") == 0) return TOKEN_STRING;
    if (strcmp(str, "bool") == 0) return TOKEN_BOOL;
    if (strcmp(str, "true") == 0) return TOKEN_TRUE;
    if (strcmp(str, "false") == 0) return TOKEN_FALSE;
    if (strcmp(str, "null") == 0) return TOKEN_NULL;
    return TOKEN_ERROR;
}

Lexer* lexer_init(const char* source) {
    Lexer* lexer = malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
    
    return lexer;
}

void lexer_free(Lexer* lexer) {
    free(lexer);
}

static bool is_at_end(Lexer* lexer) {
    return lexer->source[lexer->current] == '\0';
}

static char advance(Lexer* lexer) {
    char c = lexer->source[lexer->current++];
    lexer->column++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    }
    return c;
}

static char peek(Lexer* lexer) {
    return lexer->source[lexer->current];
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current + 1];
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (lexer->source[lexer->current] != expected) return false;
    
    lexer->current++;
    lexer->column++;
    return true;
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;
    token.column = lexer->column - (lexer->current - lexer->start);
    
    size_t length = lexer->current - lexer->start;
    token.lexeme = malloc(length + 1);
    if (token.lexeme) {
        strncpy(token.lexeme, lexer->source + lexer->start, length);
        token.lexeme[length] = '\0';
    }
    
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.lexeme = strdup(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static void skip_whitespace(Lexer* lexer) {
    while (true) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                advance(lexer);
                break;
            case '#':
                while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                break;
            default:
                return;
        }
    }
}

static Token string(Lexer* lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') lexer->line++;
        advance(lexer);
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string.");
    }
    
    advance(lexer);  // The closing "
    
    return make_token(lexer, TOKEN_STRING_LITERAL);
}

static Token number(Lexer* lexer) {
    while (isdigit(peek(lexer))) advance(lexer);
    
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer);  // Consume the '.'
        
        while (isdigit(peek(lexer))) advance(lexer);
        return make_token(lexer, TOKEN_FLOAT_LITERAL);
    }
    
    return make_token(lexer, TOKEN_INTEGER);
}

static Token identifier(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') advance(lexer);
    
    Token token = make_token(lexer, TOKEN_IDENTIFIER);
    if (is_keyword(token.lexeme)) {
        token.type = keyword_to_token(token.lexeme);
    }
    
    return token;
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance(lexer);
    
    if (isalpha(c) || c == '_') return identifier(lexer);
    if (isdigit(c)) return number(lexer);
    
    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': return match(lexer, '.') ? make_token(lexer, TOKEN_DOT_DOT) : make_token(lexer, TOKEN_DOT);
        case '-': return match(lexer, '>') ? make_token(lexer, TOKEN_ARROW) : make_token(lexer, TOKEN_MINUS);
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '/': return make_token(lexer, TOKEN_DIVIDE);
        case '*': return make_token(lexer, TOKEN_MULTIPLY);
        case '%': return make_token(lexer, TOKEN_MODULO);
        case '!': return match(lexer, '=') ? make_token(lexer, TOKEN_NOT_EQUALS) : make_token(lexer, TOKEN_NOT);
        case '=': return match(lexer, '=') ? make_token(lexer, TOKEN_EQUALS) : make_token(lexer, TOKEN_ASSIGN);
        case '<': return match(lexer, '=') ? make_token(lexer, TOKEN_LESS_EQUALS) : make_token(lexer, TOKEN_LESS);
        case '>': return match(lexer, '=') ? make_token(lexer, TOKEN_GREATER_EQUALS) : make_token(lexer, TOKEN_GREATER);
        case '"': return string(lexer);
        case ':': return make_token(lexer, TOKEN_COLON);
    }
    
    return error_token(lexer, "Unexpected character.");
}

Token lexer_peek_token(Lexer* lexer) {
    size_t start = lexer->start;
    size_t current = lexer->current;
    size_t line = lexer->line;
    size_t column = lexer->column;
    
    Token token = lexer_next_token(lexer);
    
    lexer->start = start;
    lexer->current = current;
    lexer->line = line;
    lexer->column = column;
    
    return token;
} 