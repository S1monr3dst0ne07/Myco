#include "../include/parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_equality(Parser* parser);
static ASTNode* parse_comparison(Parser* parser);
static ASTNode* parse_term(Parser* parser);
static ASTNode* parse_factor(Parser* parser);
static ASTNode* parse_unary(Parser* parser);
static ASTNode* parse_primary(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_block(Parser* parser);
static ASTNode* parse_function_def(Parser* parser);
static ASTNode* parse_var_decl(Parser* parser);
static ASTNode* parse_if_stmt(Parser* parser);
static ASTNode* parse_while_stmt(Parser* parser);
static ASTNode* parse_for_stmt(Parser* parser);
static ASTNode* parse_switch_stmt(Parser* parser);
static ASTNode* parse_try_stmt(Parser* parser);
static ASTNode* parse_print_stmt(Parser* parser);
static ASTNode* parse_concat(Parser* parser);

Parser* parser_init(Lexer* lexer) {
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    parser->had_error = false;
    parser->current = lexer_next_token(lexer);
    parser->previous = parser->current;
    
    return parser;
}

void parser_free(Parser* parser) {
    free(parser);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) {
        advance(parser);
        return;
    }
    
    parser->had_error = true;
    fprintf(stderr, "Error at line %d: %s\n", parser->current.line, message);
}

static ASTNode* make_node(NodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->next = NULL;
    return node;
}

static ASTNode* parse_expression(Parser* parser) {
    return parse_concat(parser);
}

static ASTNode* parse_term(Parser* parser) {
    ASTNode* node = parse_factor(parser);
    while (parser->current.type == TOKEN_PLUS || parser->current.type == TOKEN_MINUS) {
        TokenType op = parser->current.type;
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode* right = parse_factor(parser);
        ASTNode* bin = make_node(NODE_BINARY_OP);
        bin->as.binary.left = node;
        bin->as.binary.right = right;
        bin->as.binary.operator = op;
        node = bin;
    }
    return node;
}

static ASTNode* parse_factor(Parser* parser) {
    ASTNode* node = parse_primary(parser);
    while (parser->current.type == TOKEN_MULTIPLY || parser->current.type == TOKEN_DIVIDE) {
        TokenType op = parser->current.type;
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode* right = parse_primary(parser);
        ASTNode* bin = make_node(NODE_BINARY_OP);
        bin->as.binary.left = node;
        bin->as.binary.right = right;
        bin->as.binary.operator = op;
        node = bin;
    }
    return node;
}

static ASTNode* parse_primary(Parser* parser) {
    if (parser->current.type == TOKEN_INTEGER) {
        ASTNode* node = make_node(NODE_LITERAL);
        node->as.literal.value_type = TOKEN_INTEGER;
        node->as.literal.int_value = atoll(parser->current.lexeme);
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return node;
    }
    if (parser->current.type == TOKEN_STRING_LITERAL) {
        ASTNode* node = make_node(NODE_LITERAL);
        node->as.literal.value_type = TOKEN_STRING_LITERAL;
        // Remove quotes and copy string
        char* str = parser->current.lexeme;
        str++; // Skip opening quote
        size_t len = strlen(str) - 1; // Remove closing quote
        node->as.literal.string_value = malloc(len + 1);
        strncpy(node->as.literal.string_value, str, len);
        node->as.literal.string_value[len] = '\0';
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return node;
    }
    if (parser->current.type == TOKEN_TRUE || parser->current.type == TOKEN_FALSE) {
        ASTNode* node = make_node(NODE_LITERAL);
        node->as.literal.value_type = parser->current.type;
        node->as.literal.bool_value = (parser->current.type == TOKEN_TRUE);
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return node;
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        ASTNode* node = make_node(NODE_IDENTIFIER);
        node->as.literal.string_value = strdup(parser->current.lexeme);
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return node;
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode* expr = parse_expression(parser);
        if (parser->current.type != TOKEN_RPAREN) {
            fprintf(stderr, "Expected ')'\n");
            parser->had_error = true;
            return NULL;
        }
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return expr;
    }
    fprintf(stderr, "Unexpected token in expression\n");
    parser->had_error = true;
    return NULL;
}

static ASTNode* parse_statement(Parser* parser) {
    if (parser->current.type == TOKEN_LET) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return parse_var_decl(parser);
    }
    if (parser->current.type == TOKEN_IDENTIFIER && strcmp(parser->current.lexeme, "print") == 0) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return parse_print_stmt(parser);
    }
    if (parser->current.type == TOKEN_IDENTIFIER && strcmp(parser->current.lexeme, "if") == 0) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return parse_if_stmt(parser);
    }
    if (parser->current.type == TOKEN_IDENTIFIER && strcmp(parser->current.lexeme, "while") == 0) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return parse_while_stmt(parser);
    }
    if (parser->current.type == TOKEN_IDENTIFIER && strcmp(parser->current.lexeme, "for") == 0) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return parse_for_stmt(parser);
    }
    // Fallback: try to parse as expression
    return parse_expression(parser);
}

ASTNode* parser_parse(Parser* parser) {
    ASTNode* program = make_node(NODE_PROGRAM);
    ASTNode* current = NULL;
    
    while (!check(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) break;
        
        if (!program->as.block.statements) {
            program->as.block.statements = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
        
        if (!match(parser, TOKEN_SEMICOLON)) {
            consume(parser, TOKEN_SEMICOLON, "Expect ';' after statement.");
        }
    }
    
    return program;
}

void ast_node_free(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_BINARY_OP:
            ast_node_free(node->as.binary.left);
            ast_node_free(node->as.binary.right);
            break;
            
        case NODE_UNARY_OP:
            ast_node_free(node->as.unary.operand);
            break;
            
        case NODE_LITERAL:
            if (node->as.literal.value_type == TOKEN_STRING_LITERAL) {
                free(node->as.literal.string_value);
            }
            break;
            
        case NODE_VAR_DECL:
            free(node->as.var_decl.name);
            ast_node_free(node->as.var_decl.initializer);
            ast_node_free(node->as.var_decl.type_annotation);
            break;
            
        case NODE_FUNCTION_DEF:
            free(node->as.function.name);
            ast_node_free(node->as.function.params);
            ast_node_free(node->as.function.return_type);
            ast_node_free(node->as.function.body);
            break;
            
        case NODE_IF_STMT:
            ast_node_free(node->as.if_stmt.condition);
            ast_node_free(node->as.if_stmt.then_branch);
            ast_node_free(node->as.if_stmt.else_branch);
            break;
            
        case NODE_BLOCK:
            ast_node_free(node->as.block.statements);
            break;
    }
    
    ast_node_free(node->next);
    free(node);
}

// Minimal stub implementations for unimplemented parser functions
static ASTNode* parse_var_decl(Parser* parser) {
    // Assume 'let' already matched
    ASTNode* node = make_node(NODE_VAR_DECL);
    if (parser->current.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier after 'let'\n");
        parser->had_error = true;
        return NULL;
    }
    node->as.var_decl.name = strdup(parser->current.lexeme);
    // Advance past identifier
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Expect '='
    if (parser->current.type != TOKEN_ASSIGN) {
        fprintf(stderr, "Expected '=' after variable name\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse initializer expression
    node->as.var_decl.initializer = parse_expression(parser);
    node->as.var_decl.is_const = 0;
    return node;
}

static ASTNode* parse_function_def(Parser* parser) {
    ASTNode* node = make_node(NODE_FUNCTION_DEF);
    // Expect function name
    if (parser->current.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected function name\n");
        parser->had_error = true;
        return NULL;
    }
    node->as.function.name = strdup(parser->current.lexeme);
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Expect opening parenthesis
    if (parser->current.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after function name\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse parameters
    node->as.function.params = NULL;
    if (parser->current.type != TOKEN_RPAREN) {
        do {
            if (parser->current.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Expected parameter name\n");
                parser->had_error = true;
                return NULL;
            }
            ASTNode* param = make_node(NODE_IDENTIFIER);
            param->as.literal.string_value = strdup(parser->current.lexeme);
            param->next = node->as.function.params;
            node->as.function.params = param;
            parser->previous = parser->current;
            parser->current = lexer_next_token(parser->lexer);
        } while (match(parser, TOKEN_COMMA));
    }
    // Expect closing parenthesis
    if (parser->current.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after parameters\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Expect colon
    if (parser->current.type != TOKEN_COLON) {
        fprintf(stderr, "Expected ':' after function definition\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse function body
    node->as.function.body = parse_block(parser);
    return node;
}

static ASTNode* parse_if_stmt(Parser* parser) {
    // Assume 'if' already matched
    ASTNode* node = make_node(NODE_IF_STMT);
    
    // Parse condition
    if (parser->current.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after 'if'\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    node->as.if_stmt.condition = parse_expression(parser);
    
    if (parser->current.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after if condition\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    // Parse then branch
    if (parser->current.type != TOKEN_LBRACE) {
        fprintf(stderr, "Expected '{' after if condition\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    node->as.if_stmt.then_branch = parse_block(parser);
    
    // Parse else branch if present
    if (parser->current.type == TOKEN_ELSE) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        
        if (parser->current.type != TOKEN_LBRACE) {
            fprintf(stderr, "Expected '{' after else\n");
            parser->had_error = true;
            return NULL;
        }
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        
        node->as.if_stmt.else_branch = parse_block(parser);
    } else {
        node->as.if_stmt.else_branch = NULL;
    }
    
    return node;
}

static ASTNode* parse_while_stmt(Parser* parser) {
    // Assume 'while' already matched
    ASTNode* node = make_node(NODE_WHILE_STMT);
    
    // Parse condition
    if (parser->current.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after 'while'\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    node->as.while_stmt.condition = parse_expression(parser);
    
    if (parser->current.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after while condition\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    // Parse body
    if (parser->current.type != TOKEN_LBRACE) {
        fprintf(stderr, "Expected '{' after while condition\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    node->as.while_stmt.body = parse_block(parser);
    
    return node;
}

static ASTNode* parse_for_stmt(Parser* parser) {
    ASTNode* node = make_node(NODE_FOR_STMT);
    // Expect variable name
    if (parser->current.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected variable name in for loop\n");
        parser->had_error = true;
        return NULL;
    }
    node->as.for_stmt.var_name = strdup(parser->current.lexeme);
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Expect 'in'
    if (parser->current.type != TOKEN_IDENTIFIER || strcmp(parser->current.lexeme, "in") != 0) {
        fprintf(stderr, "Expected 'in' in for loop\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse range
    node->as.for_stmt.start = parse_expression(parser);
    if (parser->current.type != TOKEN_COLON) {
        fprintf(stderr, "Expected ':' in for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    node->as.for_stmt.end = parse_expression(parser);
    // Expect colon
    if (parser->current.type != TOKEN_COLON) {
        fprintf(stderr, "Expected ':' after for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse loop body
    node->as.for_stmt.body = parse_block(parser);
    return node;
}

static ASTNode* parse_switch_stmt(Parser* parser) { return NULL; }
static ASTNode* parse_try_stmt(Parser* parser) { return NULL; }

// Minimal parse for print(expr);
static ASTNode* parse_print_stmt(Parser* parser) {
    // Assume 'print' already matched
    if (parser->current.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after 'print'\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    ASTNode* expr = parse_expression(parser);
    
    if (parser->current.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after print expression\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    
    ASTNode* node = make_node(NODE_FUNCTION_CALL);
    node->as.function.name = strdup("print");
    node->as.function.params = expr;
    return node;
}

static ASTNode* parse_block(Parser* parser) {
    ASTNode* block = make_node(NODE_BLOCK);
    ASTNode* current = NULL;
    
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) break;
        
        if (!block->as.block.statements) {
            block->as.block.statements = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
        
        if (!match(parser, TOKEN_SEMICOLON)) {
            consume(parser, TOKEN_SEMICOLON, "Expect ';' after statement.");
        }
    }
    
    if (!match(parser, TOKEN_RBRACE)) {
        consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
    }
    
    return block;
}

// Parse concatenation (.. operator)
static ASTNode* parse_concat(Parser* parser) {
    ASTNode* node = parse_term(parser);
    while (parser->current.type == TOKEN_DOT_DOT) {
        TokenType op = parser->current.type;
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode* right = parse_term(parser);
        ASTNode* bin = make_node(NODE_BINARY_OP);
        bin->as.binary.left = node;
        bin->as.binary.right = right;
        bin->as.binary.operator = op;
        node = bin;
    }
    return node;
} 