#ifndef LEXER_H
#define LEXER_H

#include "myco.h"

// Token and Lexer structs are defined in myco.h

void lexer_init(Lexer *lexer, const char *src);
Token lexer_next(Lexer *lexer);
const char *token_type_to_string(TokenType type);

#endif // LEXER_H 