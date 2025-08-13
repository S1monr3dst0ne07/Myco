#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// Helper function to check if an identifier is a keyword
static MycoTokenType get_keyword_type(const char* text) {
    if (strcmp(text, "func") == 0) return TOKEN_FUNC;
    if (strcmp(text, "let") == 0) return TOKEN_LET;
    if (strcmp(text, "if") == 0) return TOKEN_IF;
    if (strcmp(text, "else") == 0) return TOKEN_ELSE;
    if (strcmp(text, "for") == 0) return TOKEN_FOR;
    if (strcmp(text, "while") == 0) return TOKEN_WHILE;
    if (strcmp(text, "end") == 0) return TOKEN_END;
    if (strcmp(text, "return") == 0) return TOKEN_RETURN;
    if (strcmp(text, "switch") == 0) return TOKEN_SWITCH;
    if (strcmp(text, "case") == 0) return TOKEN_CASE;
    if (strcmp(text, "default") == 0) return TOKEN_DEFAULT;
    if (strcmp(text, "try") == 0) return TOKEN_TRY;
    if (strcmp(text, "catch") == 0) return TOKEN_CATCH;
    if (strcmp(text, "print") == 0) return TOKEN_PRINT;
    if (strcmp(text, "in") == 0) return TOKEN_IN;
    if (strcmp(text, "use") == 0) return TOKEN_USE;
    if (strcmp(text, "as") == 0) return TOKEN_AS;
    if (strcmp(text, "int") == 0) return TOKEN_TYPE_MARKER;
    if (strcmp(text, "string") == 0) return TOKEN_STRING_TYPE;
    return TOKEN_IDENTIFIER;
}

#define MAX_TOKENS 1000

Token* lexer_tokenize(const char* source) {
    Token* tokens = (Token*)malloc(MAX_TOKENS * sizeof(Token));
    if (!tokens) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    int token_count = 0;
    int line = 1;
    const char* p = source;

    while (*p && token_count < MAX_TOKENS) {
        // Skip whitespace
        if (isspace(*p)) {
            if (*p == '\n') line++;
            p++;
            continue;
        }

        // Skip single-line comments (starting with #)
        if (*p == '#') {
            while (*p && *p != '\n') p++;
            continue;
        }

        // Skip multi-line comments (enclosed in /* and */)
        if (*p == '/' && *(p + 1) == '*') {
            p += 2; // Skip /*
            while (*p && !(*p == '*' && *(p + 1) == '/')) {
                if (*p == '\n') line++;
                p++;
            }
            if (*p) p += 2; // Skip */
            continue;
        }

        // PATH starting with ./
        if (*p == '.' && *(p + 1) == '/') {
            const char* start = p;
            p += 2;
            while (*p && !isspace(*p)) p++;
            int len = p - start;
            char* text = (char*)malloc(len + 1);
            strncpy(text, start, len); text[len] = '\0';
            tokens[token_count].type = TOKEN_PATH;
            tokens[token_count].text = text;
            tokens[token_count].line = line;
            token_count++;
            continue;
        }

        // Single dot for member access
        if (*p == '.' && *(p + 1) != '/' && *(p + 1) != '.') {
            tokens[token_count].type = TOKEN_DOT;
            tokens[token_count].text = strdup(".");
            tokens[token_count].line = line;
            token_count++;
            p++;
            continue;
        }

        // Keywords and identifiers
        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            while (isalnum(*p) || *p == '_' || *p == '-') p++;
            int len = p - start;
            char* text = (char*)malloc(len + 1);
            strncpy(text, start, len);
            text[len] = '\0';

            // Check for keywords
            if (strcmp(text, "func") == 0) tokens[token_count].type = TOKEN_FUNC;
            else if (strcmp(text, "let") == 0) tokens[token_count].type = TOKEN_LET;
            else if (strcmp(text, "if") == 0) tokens[token_count].type = TOKEN_IF;
            else if (strcmp(text, "else") == 0) tokens[token_count].type = TOKEN_ELSE;
            else if (strcmp(text, "for") == 0) tokens[token_count].type = TOKEN_FOR;
            else if (strcmp(text, "while") == 0) tokens[token_count].type = TOKEN_WHILE;
            else if (strcmp(text, "end") == 0) tokens[token_count].type = TOKEN_END;
            else if (strcmp(text, "return") == 0) tokens[token_count].type = TOKEN_RETURN;
            else if (strcmp(text, "switch") == 0) tokens[token_count].type = TOKEN_SWITCH;
            else if (strcmp(text, "case") == 0) tokens[token_count].type = TOKEN_CASE;
            else if (strcmp(text, "default") == 0) tokens[token_count].type = TOKEN_DEFAULT;
            else if (strcmp(text, "try") == 0) tokens[token_count].type = TOKEN_TRY;
            else if (strcmp(text, "catch") == 0) tokens[token_count].type = TOKEN_CATCH;
            else if (strcmp(text, "print") == 0) tokens[token_count].type = TOKEN_PRINT;
            else if (strcmp(text, "int") == 0) tokens[token_count].type = TOKEN_TYPE_MARKER;
            else if (strcmp(text, "string") == 0) tokens[token_count].type = TOKEN_STRING_TYPE;
            else if (strcmp(text, "in") == 0) tokens[token_count].type = TOKEN_IN;
            else if (strcmp(text, "use") == 0) tokens[token_count].type = TOKEN_USE;
            else if (strcmp(text, "as") == 0) tokens[token_count].type = TOKEN_AS;
            else tokens[token_count].type = TOKEN_IDENTIFIER;

            tokens[token_count].text = text;
            tokens[token_count].line = line;
            token_count++;
            continue;
        }

        // Numbers
        if (isdigit(*p)) {
            const char* start = p;
            while (isdigit(*p)) p++;
            int len = p - start;
            char* text = (char*)malloc(len + 1);
            strncpy(text, start, len);
            text[len] = '\0';
            tokens[token_count].type = TOKEN_NUMBER;
            tokens[token_count].text = text;
            tokens[token_count].line = line;
            token_count++;
            continue;
        }

        // Strings with simple escapes (\n, \t, \", \\) enclosed in double quotes
        if (*p == '"') {
            p++; // skip opening quote
            char* buf = NULL; int cap = 0; int len = 0;
            while (*p) {
                if (*p == '"') { // closing quote
                    p++;
                    break;
                }
                char ch = *p++;
                if (ch == '\\' && *p) {
                    char esc = *p++;
                    switch (esc) {
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        case '\\': ch = '\\'; break;
                        case '"': ch = '"'; break;
                        default: ch = esc; break;
                    }
                }
                if (len + 1 >= cap) { cap = cap ? cap * 2 : 32; buf = (char*)realloc(buf, cap); }
                buf[len++] = ch;
            }
            if (!buf) { buf = (char*)malloc(1); buf[0] = '\0'; }
            if (*(p-1) != '"') { // no closing quote found
                fprintf(stderr, "Error: Unterminated string at line %d\n", line);
                free(tokens);
                free(buf);
                return NULL;
            }
            if (len + 1 >= cap) { cap = len + 1; buf = (char*)realloc(buf, cap); }
            buf[len] = '\0';
            tokens[token_count].type = TOKEN_STRING;
            tokens[token_count].text = buf;
            tokens[token_count].line = line;
            token_count++;
            continue;
        }

        // Operators and symbols
        switch (*p) {
            case '+': tokens[token_count].type = TOKEN_OPERATOR; tokens[token_count].text = strdup("+"); break;
            case '-': tokens[token_count].type = TOKEN_OPERATOR; tokens[token_count].text = strdup("-"); break;
            case '*': tokens[token_count].type = TOKEN_OPERATOR; tokens[token_count].text = strdup("*"); break;
            case '/': tokens[token_count].type = TOKEN_OPERATOR; tokens[token_count].text = strdup("/"); break;
            case '%': tokens[token_count].type = TOKEN_OPERATOR; tokens[token_count].text = strdup("%"); break;
            case ':': tokens[token_count].type = TOKEN_COLON; tokens[token_count].text = strdup(":"); break;
            case ';': tokens[token_count].type = TOKEN_SEMICOLON; tokens[token_count].text = strdup(";"); break;
            case '(': tokens[token_count].type = TOKEN_LPAREN; tokens[token_count].text = strdup("("); break;
            case ')': tokens[token_count].type = TOKEN_RPAREN; tokens[token_count].text = strdup(")"); break;
            case ',': tokens[token_count].type = TOKEN_COMMA; tokens[token_count].text = strdup(","); break;
            case '<':
                if (*(p + 1) == '=') {
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].text = strdup("<=");
                    p++;
                } else {
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].text = strdup("<");
                }
                break;
            case '>':
                if (*(p + 1) == '=') {
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].text = strdup(">=");
                    p++;
                } else {
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].text = strdup(">");
                }
                break;
            case '!':
                if (*(p + 1) == '=') {
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].text = strdup("!=");
                    p++;
                } else {
                    fprintf(stderr, "Error: Unknown character '%c' at line %d\n", *p, line);
                    free(tokens);
                    return NULL;
                }
                break;
            case '=':
                if (*(p + 1) == '=') {
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].text = strdup("==");
                    p++;
                } else {
                    tokens[token_count].type = TOKEN_ASSIGN;
                    tokens[token_count].text = strdup("=");
                }
                break;
            default:
                fprintf(stderr, "Error: Unknown character '%c' at line %d\n", *p, line);
                free(tokens);
                return NULL;
        }
        tokens[token_count].line = line;
        token_count++;
        p++;
    }

    // Add EOF token
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].text = NULL;
    tokens[token_count].line = line;

    return tokens;
}

void lexer_free_tokens(Token* tokens) {
    if (!tokens) return;
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].text);
    }
    free(tokens);
}
