#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

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

        // Keywords and identifiers
        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            while (isalnum(*p) || *p == '_') p++;
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
            else if (strcmp(text, "int") == 0) tokens[token_count].type = TOKEN_TYPE;
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

        // Strings (assuming strings are enclosed in double quotes)
        if (*p == '"') {
            p++;
            const char* start = p;
            while (*p && *p != '"') p++;
            if (*p == '"') {
                int len = p - start;
                char* text = (char*)malloc(len + 1);
                strncpy(text, start, len);
                text[len] = '\0';
                tokens[token_count].type = TOKEN_STRING;
                tokens[token_count].text = text;
                tokens[token_count].line = line;
                token_count++;
                p++;
                continue;
            } else {
                fprintf(stderr, "Error: Unterminated string at line %d\n", line);
                free(tokens);
                return NULL;
            }
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
