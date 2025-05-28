#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "myco.h"

static struct {
    const char* keyword;
    TokenType type;
} keywords[] = {
    {"let", TOKEN_LET}, {"const", TOKEN_CONST}, {"var", TOKEN_VAR},
    {"func", TOKEN_FUNC}, {"return", TOKEN_RETURN},
    {"if", TOKEN_IF}, {"else", TOKEN_ELSE}, {"elseif", TOKEN_ELSEIF},
    {"while", TOKEN_WHILE}, {"for", TOKEN_FOR},
    {"in", TOKEN_IN},
    {"switch", TOKEN_SWITCH}, {"case", TOKEN_CASE}, {"default", TOKEN_DEFAULT},
    {"try", TOKEN_TRY}, {"catch", TOKEN_CATCH}, {"end", TOKEN_END},
    {"print", TOKEN_PRINT}, {"import", TOKEN_IMPORT}, {"async", TOKEN_ASYNC}, {"none", TOKEN_NONE},
    {"int", TOKEN_INT}, {"float", TOKEN_FLOAT}, {"str", TOKEN_STR}, {"bool", TOKEN_BOOL},
    {"list", TOKEN_LIST}, {"map", TOKEN_MAP},
    {NULL, TOKEN_EOF}
};

// Add a flag to track if a semicolon should be emitted for a newline
static int pending_newline_semicolon = 0;

// Track the last token emitted by the lexer
static Token previous_token;

// Helper function to determine if a token type is a statement terminator
static bool is_statement_terminator(TokenType type) {
    switch (type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_IDENTIFIER:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_RIGHT_BRACKET:
        case TOKEN_RIGHT_BRACE:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NONE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        case TOKEN_SEMICOLON:
        case TOKEN_END:
        case TOKEN_ELSE:
        case TOKEN_ELSEIF:
        case TOKEN_CATCH:
        case TOKEN_CASE:
        case TOKEN_DEFAULT:
            return true;
        default:
            return false;
    }
}

// Helper function to determine if a token type is a block header token
static bool is_block_header(TokenType type) {
    switch (type) {
        case TOKEN_END:
        case TOKEN_ELSE:
        case TOKEN_ELSEIF:
        case TOKEN_CATCH:
        case TOKEN_CASE:
        case TOKEN_DEFAULT:
        case TOKEN_COLON:  // Add colon as a block header
            return true;
        default:
            return false;
    }
}

static int is_block_header_start(const char* s) {
    // Check for block header keywords at the start of s
    static const char* keywords[] = {"end", "else", "elseif", "catch", "case", "default", NULL};
    for (int i = 0; keywords[i]; i++) {
        size_t len = strlen(keywords[i]);
        if (strncmp(s, keywords[i], len) == 0 &&
            (s[len] == '\0' || s[len] == ' ' || s[len] == '\n' || s[len] == '\r' || s[len] == '\t' || s[len] == ':')) {
            return 1;
        }
    }
    return 0;
}

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->pos = 0;
    lexer->line = 1;
    lexer->col = 1;
    // Initialize previous_token to a default value
    previous_token.type = TOKEN_EOF;
    previous_token.start = NULL;
    previous_token.length = 0;
    previous_token.line = 0;
    previous_token.col = 0;
}

static bool is_at_end(Lexer* lexer) {
    return lexer->pos >= lexer->length;
}

static char peek(Lexer* lexer) {
    return is_at_end(lexer) ? '\0' : lexer->source[lexer->pos];
}

static char peek_next(Lexer* lexer) {
    return (lexer->pos + 1 >= lexer->length) ? '\0' : lexer->source[lexer->pos + 1];
}

static char advance(Lexer* lexer) {
    char c = lexer->source[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer) || lexer->source[lexer->pos] != expected) return false;
    lexer->pos++;
    lexer->col++;
    return true;
}

static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ': case '\r': case '\t':
                advance(lexer);
                break;
            case '\n':
                advance(lexer);
                break;
            case '#': // comment
                while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer* lexer, TokenType type, int start, int length) {
    Token token;
    token.type = type;
    token.start = lexer->source + start;
    token.length = length;
    token.line = lexer->line;
    token.col = lexer->col - length;
    return token;
}

static Token identifier(Lexer* lexer, int start) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') advance(lexer);
    int length = lexer->pos - start;
    for (int i = 0; keywords[i].keyword; i++) {
        if (length == (int)strlen(keywords[i].keyword) &&
            strncmp(lexer->source + start, keywords[i].keyword, length) == 0) {
            return make_token(lexer, keywords[i].type, start, length);
        }
    }
    return make_token(lexer, TOKEN_IDENTIFIER, start, length);
}

static Token number(Lexer* lexer, int start) {
    while (isdigit(peek(lexer))) advance(lexer);
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer);
        while (isdigit(peek(lexer))) advance(lexer);
    }
    int length = lexer->pos - start;
    return make_token(lexer, TOKEN_NUMBER, start, length);
}

static Token string(Lexer* lexer, int start) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') advance(lexer);
        else advance(lexer);
    }
    if (is_at_end(lexer)) return make_token(lexer, TOKEN_ERROR, start, lexer->pos - start);
    advance(lexer); // closing quote
    int length = lexer->pos - start;
    return make_token(lexer, TOKEN_STRING, start, length);
}

static char peek_non_whitespace(Lexer* lexer) {
    int pos = lexer->pos;
    while (pos < lexer->length) {
        char c = lexer->source[pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
            continue;
        }
        return c;
    }
    return '\0';
}

Token lexer_next_token(Lexer* lexer) {
    if (pending_newline_semicolon) {
        pending_newline_semicolon = 0;
        return make_token(lexer, TOKEN_SEMICOLON, lexer->pos, 0);
    }
    
    skip_whitespace(lexer);
    int start = lexer->pos;
    if (is_at_end(lexer)) return make_token(lexer, TOKEN_EOF, start, 0);
    
    char c = advance(lexer);
    Token token;
    
    if (isalpha(c) || c == '_') {
        token = identifier(lexer, start);
    } else if (isdigit(c)) {
        token = number(lexer, start);
    } else {
        switch (c) {
            case '\n': {
                if (is_statement_terminator(previous_token.type)) {
                    pending_newline_semicolon = 1;
                }
                lexer->line++;
                break;
            }
            case '(': token = make_token(lexer, TOKEN_LEFT_PAREN, start, 1); break;
            case ')': token = make_token(lexer, TOKEN_RIGHT_PAREN, start, 1); break;
            case '{': token = make_token(lexer, TOKEN_LEFT_BRACE, start, 1); break;
            case '}': token = make_token(lexer, TOKEN_RIGHT_BRACE, start, 1); break;
            case '[': token = make_token(lexer, TOKEN_LEFT_BRACKET, start, 1); break;
            case ']': token = make_token(lexer, TOKEN_RIGHT_BRACKET, start, 1); break;
            case ',': token = make_token(lexer, TOKEN_COMMA, start, 1); break;
            case '.': 
                if (match(lexer, '.')) token = make_token(lexer, TOKEN_DOT_DOT, start, 2);
                else token = make_token(lexer, TOKEN_DOT, start, 1);
                break;
            case ':': token = make_token(lexer, TOKEN_COLON, start, 1); break;
            case ';': token = make_token(lexer, TOKEN_SEMICOLON, start, 1); break;
            case '+':
                if (match(lexer, '+')) token = make_token(lexer, TOKEN_PLUS_PLUS, start, 2);
                else if (match(lexer, '=')) token = make_token(lexer, TOKEN_PLUS_ASSIGN, start, 2);
                else token = make_token(lexer, TOKEN_PLUS, start, 1);
                break;
            case '-':
                if (match(lexer, '-')) token = make_token(lexer, TOKEN_MINUS_MINUS, start, 2);
                else if (match(lexer, '=')) token = make_token(lexer, TOKEN_MINUS_ASSIGN, start, 2);
                else token = make_token(lexer, TOKEN_MINUS, start, 1);
                break;
            case '*':
                if (match(lexer, '=')) token = make_token(lexer, TOKEN_STAR_ASSIGN, start, 2);
                else token = make_token(lexer, TOKEN_STAR, start, 1);
                break;
            case '/':
                if (match(lexer, '=')) token = make_token(lexer, TOKEN_SLASH_ASSIGN, start, 2);
                else token = make_token(lexer, TOKEN_SLASH, start, 1);
                break;
            case '%': token = make_token(lexer, TOKEN_PERCENT, start, 1); break;
            case '=':
                if (match(lexer, '=')) token = make_token(lexer, TOKEN_EQUAL, start, 2);
                else token = make_token(lexer, TOKEN_ASSIGN, start, 1);
                break;
            case '!':
                if (match(lexer, '=')) token = make_token(lexer, TOKEN_BANG_EQUAL, start, 2);
                else token = make_token(lexer, TOKEN_BANG, start, 1);
                break;
            case '<':
                if (match(lexer, '=')) token = make_token(lexer, TOKEN_LESS_EQUAL, start, 2);
                else token = make_token(lexer, TOKEN_LESS, start, 1);
                break;
            case '>':
                if (match(lexer, '=')) token = make_token(lexer, TOKEN_GREATER_EQUAL, start, 2);
                else token = make_token(lexer, TOKEN_GREATER, start, 1);
                break;
            case '"': token = string(lexer, start); break;
            default:
                fprintf(stderr, "[Line %d] Error: Unexpected character\n", lexer->line);
                exit(1);
        }
    }
    
    previous_token = token;
    return token;
} 