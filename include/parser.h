#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    Token current;
    Token previous;
    bool had_error;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
ASTNode *parse(Parser *parser);
void parser_free(Parser *parser);

#endif // PARSER_H 