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
static ASTNode* parse_return_stmt(Parser* parser);

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
    fprintf(stderr, "Parsing primary: token type %d, lexeme '%s', line %d\n", parser->current.type, parser->current.lexeme, parser->current.line);
    
    // Check for control flow keywords that should not be parsed as expressions
    if (parser->current.type == TOKEN_IDENTIFIER) {
        const char* keyword = parser->current.lexeme;
        if (strcmp(keyword, "if") == 0 || 
            strcmp(keyword, "for") == 0 || 
            strcmp(keyword, "while") == 0 || 
            strcmp(keyword, "else") == 0 || 
            strcmp(keyword, "catch") == 0 ||
            strcmp(keyword, "switch") == 0 ||
            strcmp(keyword, "default") == 0) {
            fprintf(stderr, "Unexpected control flow keyword '%s' in expression\n", keyword);
            parser->had_error = true;
            return NULL;
        }
    }
    
    if (parser->current.type == TOKEN_INTEGER) {
        ASTNode* node = make_node(NODE_LITERAL);
        node->as.literal.value_type = TOKEN_INTEGER;
        node->as.literal.int_value = atoll(parser->current.lexeme);
        advance(parser);
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
        advance(parser);
        return node;
    }
    if (parser->current.type == TOKEN_TRUE || parser->current.type == TOKEN_FALSE) {
        ASTNode* node = make_node(NODE_LITERAL);
        node->as.literal.value_type = parser->current.type;
        node->as.literal.bool_value = (parser->current.type == TOKEN_TRUE);
        advance(parser);
        return node;
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        ASTNode* node = make_node(NODE_IDENTIFIER);
        node->as.literal.string_value = strdup(parser->current.lexeme);
        advance(parser);
        // Check if this is a function call (followed by '(')
        if (parser->current.type == TOKEN_LPAREN) {
            advance(parser);
            ASTNode* call = make_node(NODE_FUNCTION_CALL);
            call->as.function.name = node->as.literal.string_value;
            call->as.function.params = NULL;
            // Parse arguments (comma-separated list)
            if (parser->current.type != TOKEN_RPAREN) {
                do {
                    ASTNode* arg = parse_expression(parser);
                    if (!arg) {
                        parser->had_error = true;
                        return NULL;
                    }
                    arg->next = call->as.function.params;
                    call->as.function.params = arg;
                } while (match(parser, TOKEN_COMMA));
            }
            if (parser->current.type != TOKEN_RPAREN) {
                fprintf(stderr, "Expected ')' after function call arguments\n");
                parser->had_error = true;
                return NULL;
            }
            advance(parser);
            free(node);
            return call;
        }
        return node;
    }
    if (parser->current.type == TOKEN_LPAREN) {
        advance(parser);
        ASTNode* expr = parse_expression(parser);
        if (parser->current.type != TOKEN_RPAREN) {
            fprintf(stderr, "Expected ')'\n");
            parser->had_error = true;
            return NULL;
        }
        advance(parser);
        return expr;
    }
    fprintf(stderr, "Unexpected token in expression: token type %d, lexeme '%s', line %d\n", parser->current.type, parser->current.lexeme, parser->current.line);
    parser->had_error = true;
    return NULL;
}

static ASTNode* parse_statement(Parser* parser) {
    fprintf(stderr, "DEBUG: TOKEN_IDENTIFIER = %d\n", TOKEN_IDENTIFIER);
    fprintf(stderr, "DEBUG: parse_statement: token type %d, lexeme '%s' (len=%zu), line %d\n", parser->current.type, parser->current.lexeme, strlen(parser->current.lexeme), parser->current.line);
    fprintf(stderr, "Parsing statement: token type %d, lexeme '%s' (len=%zu), line %d\n", parser->current.type, parser->current.lexeme, strlen(parser->current.lexeme), parser->current.line);
    
    // Return NULL for 'end' token to signal end of block
    if (parser->current.type == TOKEN_END) {
        advance(parser);  // Advance past the 'end' token before returning
        return NULL;
    }
    
    // Handle control flow keywords by their token types
    switch (parser->current.type) {
        case TOKEN_IF:
            advance(parser);
            // Expect opening parenthesis
            if (!match(parser, TOKEN_LPAREN)) {
                fprintf(stderr, "Expected '(' after 'if'\n");
                parser->had_error = true;
                return NULL;
            }
            ASTNode* if_node = parse_if_stmt(parser);
            if (!if_node) {
                // Error recovery: skip until end of statement
                while (!check(parser, TOKEN_EOF) && 
                       !check(parser, TOKEN_SEMICOLON) && 
                       !check(parser, TOKEN_RBRACE) && 
                       !check(parser, TOKEN_END)) {
                    advance(parser);
                }
            }
            return if_node;
            
        case TOKEN_FOR:
            advance(parser);
            // Expect variable name
            if (parser->current.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Expected variable name in for loop\n");
                parser->had_error = true;
                return NULL;
            }
            ASTNode* for_node = parse_for_stmt(parser);
            if (!for_node) {
                // Error recovery: skip until end of statement
                while (!check(parser, TOKEN_EOF) && 
                       !check(parser, TOKEN_SEMICOLON) && 
                       !check(parser, TOKEN_RBRACE) && 
                       !check(parser, TOKEN_END)) {
                    advance(parser);
                }
            }
            return for_node;
            
        case TOKEN_WHILE:
            advance(parser);
            // Expect opening parenthesis
            if (!match(parser, TOKEN_LPAREN)) {
                fprintf(stderr, "Expected '(' after 'while'\n");
                parser->had_error = true;
                return NULL;
            }
            ASTNode* while_node = parse_while_stmt(parser);
            if (!while_node) {
                // Error recovery: skip until end of statement
                while (!check(parser, TOKEN_EOF) && 
                       !check(parser, TOKEN_SEMICOLON) && 
                       !check(parser, TOKEN_RBRACE) && 
                       !check(parser, TOKEN_END)) {
                    advance(parser);
                }
            }
            return while_node;
            
        case TOKEN_ELSE:
            advance(parser);
            // Handle else branch
            if (parser->current.type != TOKEN_LBRACE) {
                fprintf(stderr, "Expected '{' after else\n");
                parser->had_error = true;
                return NULL;
            }
            advance(parser);
            return parse_block(parser);
            
        case TOKEN_CATCH:
            advance(parser);
            // Handle catch block
            if (parser->current.type != TOKEN_LBRACE) {
                fprintf(stderr, "Expected '{' after catch\n");
                parser->had_error = true;
                return NULL;
            }
            advance(parser);
            return parse_block(parser);
            
        case TOKEN_RETURN:
            return parse_return_stmt(parser);
            
        case TOKEN_FUNC:
            return parse_function_def(parser);
            
        case TOKEN_LET:
            advance(parser);
            return parse_var_decl(parser);
            
        case TOKEN_IDENTIFIER:
            // Handle identifier-based keywords
            if (strcmp(parser->current.lexeme, "print") == 0) {
                advance(parser);
                return parse_print_stmt(parser);
            }
            if (strcmp(parser->current.lexeme, "switch") == 0) {
                advance(parser);
                return parse_switch_stmt(parser);
            }
            if (strcmp(parser->current.lexeme, "default") == 0) {
                // Default case should only appear inside switch statements
                fprintf(stderr, "Unexpected 'default' outside switch statement\n");
                parser->had_error = true;
                return NULL;
            }
            // Fall through to expression parsing for regular identifiers
            break;
            
        default:
            break;
    }
    
    // Fallback: try to parse as expression
    return parse_expression(parser);
}

ASTNode* parser_parse(Parser* parser) {
    ASTNode* program = make_node(NODE_PROGRAM);
    ASTNode* current = NULL;
    
    while (!check(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            if (parser->had_error) {
                // On error, advance to next statement to prevent infinite loops
                while (!check(parser, TOKEN_EOF) && 
                       !check(parser, TOKEN_SEMICOLON) && 
                       !check(parser, TOKEN_RBRACE) && 
                       parser->current.type != TOKEN_END) {
                    advance(parser);
                }
                if (check(parser, TOKEN_SEMICOLON)) {
                    advance(parser);
                }
                continue;
            }
            break;
        }
        
        if (!program->as.block.statements) {
            program->as.block.statements = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
        
        // Only require semicolons in curly-brace blocks
        if (check(parser, TOKEN_SEMICOLON)) {
            advance(parser);
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
    
    // Optional type annotation
    if (parser->current.type == TOKEN_COLON) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Expected type name after ':'\n");
            parser->had_error = true;
            return NULL;
        }
        // Skip type name for now
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
    }
    
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
    fprintf(stderr, "Entering parse_function_def: token type %d, lexeme '%s', line %d\n", parser->current.type, parser->current.lexeme, parser->current.line);
    // Always advance past 'func' if present
    if (parser->current.type == TOKEN_FUNC) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        fprintf(stderr, "After advancing past 'func': token type %d, lexeme '%s', line %d\n", parser->current.type, parser->current.lexeme, parser->current.line);
    }
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
    // Parse parameters (with optional type annotations)
    node->as.function.params = NULL;
    if (parser->current.type != TOKEN_RPAREN) {
        do {
            if (parser->current.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Expected parameter name\n");
                parser->had_error = true;
                return NULL;
            }
            ASTNode* param = make_node(NODE_VAR_DECL);  // Use VAR_DECL for parameters
            param->as.var_decl.name = strdup(parser->current.lexeme);
            param->as.var_decl.initializer = NULL;  // Parameters don't have initializers
            parser->previous = parser->current;
            parser->current = lexer_next_token(parser->lexer);
            // Optional type annotation
            if (parser->current.type == TOKEN_COLON) {
                parser->previous = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_IDENTIFIER) {
                    // skip type name
                    parser->previous = parser->current;
                    parser->current = lexer_next_token(parser->lexer);
                } else {
                    fprintf(stderr, "Expected type name after ':'\n");
                    parser->had_error = true;
                    return NULL;
                }
            }
            param->next = node->as.function.params;
            node->as.function.params = param;
        } while (match(parser, TOKEN_COMMA));
    }
    // Expect closing parenthesis
    fprintf(stderr, "Before expecting ')': token type %d, lexeme '%s', line %d\n", parser->current.type, parser->current.lexeme, parser->current.line);
    if (parser->current.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after parameters\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Optional return type
    if (parser->current.type == TOKEN_ARROW) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_IDENTIFIER) {
            // skip return type name
            parser->previous = parser->current;
            parser->current = lexer_next_token(parser->lexer);
        } else {
            fprintf(stderr, "Expected return type after '->'\n");
            parser->had_error = true;
            return NULL;
        }
    }
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
    fprintf(stderr, "Exiting parse_function_def: returning node\n");
    return node;
}

static ASTNode* parse_if_stmt(Parser* parser) {
    // Assume 'if' and '(' already matched
    ASTNode* node = make_node(NODE_IF_STMT);
    
    // Parse condition
    node->as.if_stmt.condition = parse_expression(parser);
    if (!node->as.if_stmt.condition) {
        fprintf(stderr, "Expected condition expression in if statement\n");
        parser->had_error = true;
        return NULL;
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        fprintf(stderr, "Expected ')' after if condition\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse then branch
    if (!match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Expected '{' after if condition\n");
        parser->had_error = true;
        return NULL;
    }
    
    node->as.if_stmt.then_branch = parse_block(parser);
    if (!node->as.if_stmt.then_branch) {
        fprintf(stderr, "Expected block after if condition\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse else branch if present
    if (parser->current.type == TOKEN_ELSE) {
        advance(parser);
        
        if (!match(parser, TOKEN_LBRACE)) {
            fprintf(stderr, "Expected '{' after else\n");
            parser->had_error = true;
            return NULL;
        }
        
        node->as.if_stmt.else_branch = parse_block(parser);
        if (!node->as.if_stmt.else_branch) {
            fprintf(stderr, "Expected block after else\n");
            parser->had_error = true;
            return NULL;
        }
    } else {
        node->as.if_stmt.else_branch = NULL;
    }
    
    return node;
}

static ASTNode* parse_while_stmt(Parser* parser) {
    // Assume 'while' and '(' already matched
    ASTNode* node = make_node(NODE_WHILE_STMT);
    
    // Parse condition
    node->as.while_stmt.condition = parse_expression(parser);
    if (!node->as.while_stmt.condition) {
        fprintf(stderr, "Expected condition expression in while statement\n");
        parser->had_error = true;
        return NULL;
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        fprintf(stderr, "Expected ')' after while condition\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse body
    if (!match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Expected '{' after while condition\n");
        parser->had_error = true;
        return NULL;
    }
    
    node->as.while_stmt.body = parse_block(parser);
    if (!node->as.while_stmt.body) {
        fprintf(stderr, "Expected block in while statement body\n");
        parser->had_error = true;
        return NULL;
    }
    
    return node;
}

static ASTNode* parse_for_stmt(Parser* parser) {
    // Assume 'for' already matched and identifier checked
    ASTNode* node = make_node(NODE_FOR_STMT);
    node->as.for_stmt.var_name = strdup(parser->current.lexeme);
    advance(parser);
    
    // Expect 'in'
    if (!match(parser, TOKEN_IDENTIFIER) || strcmp(parser->previous.lexeme, "in") != 0) {
        fprintf(stderr, "Expected 'in' in for loop\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse range start
    node->as.for_stmt.start = parse_expression(parser);
    if (!node->as.for_stmt.start) {
        fprintf(stderr, "Expected start expression in for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Expect ':'
    if (!match(parser, TOKEN_COLON)) {
        fprintf(stderr, "Expected ':' in for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse range end
    node->as.for_stmt.end = parse_expression(parser);
    if (!node->as.for_stmt.end) {
        fprintf(stderr, "Expected end expression in for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Expect colon
    if (!match(parser, TOKEN_COLON)) {
        fprintf(stderr, "Expected ':' after for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse loop body
    if (!match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Expected '{' after for loop range\n");
        parser->had_error = true;
        return NULL;
    }
    
    node->as.for_stmt.body = parse_block(parser);
    if (!node->as.for_stmt.body) {
        fprintf(stderr, "Expected block in for loop body\n");
        parser->had_error = true;
        return NULL;
    }
    
    return node;
}

static ASTNode* parse_switch_stmt(Parser* parser) {
    // Assume 'switch' already matched
    ASTNode* node = make_node(NODE_SWITCH_STMT);
    
    // Parse value to switch on
    node->as.switch_stmt.value = parse_expression(parser);
    if (!node->as.switch_stmt.value) {
        fprintf(stderr, "Expected expression after switch\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Expect colon
    if (!match(parser, TOKEN_COLON)) {
        fprintf(stderr, "Expected ':' after switch value\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Parse cases
    node->as.switch_stmt.case_value = parse_expression(parser);
    if (!node->as.switch_stmt.case_value) {
        fprintf(stderr, "Expected case value expression\n");
        parser->had_error = true;
        return NULL;
    }
    
    if (!match(parser, TOKEN_COLON)) {
        fprintf(stderr, "Expected ':' after case value\n");
        parser->had_error = true;
        return NULL;
    }
    
    if (!match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Expected '{' after case value\n");
        parser->had_error = true;
        return NULL;
    }
    
    node->as.switch_stmt.case_body = parse_block(parser);
    if (!node->as.switch_stmt.case_body) {
        fprintf(stderr, "Expected block in case body\n");
        parser->had_error = true;
        return NULL;
    }
    
    // Expect default
    if (parser->current.type == TOKEN_IDENTIFIER && strcmp(parser->current.lexeme, "default") == 0) {
        advance(parser);
        if (!match(parser, TOKEN_COLON)) {
            fprintf(stderr, "Expected ':' after default\n");
            parser->had_error = true;
            return NULL;
        }
        
        if (!match(parser, TOKEN_LBRACE)) {
            fprintf(stderr, "Expected '{' after default\n");
            parser->had_error = true;
            return NULL;
        }
        
        node->as.switch_stmt.default_body = parse_block(parser);
        if (!node->as.switch_stmt.default_body) {
            fprintf(stderr, "Expected block in default body\n");
            parser->had_error = true;
            return NULL;
        }
    } else {
        node->as.switch_stmt.default_body = NULL;
    }
    
    return node;
}

static ASTNode* parse_try_stmt(Parser* parser) {
    ASTNode* node = make_node(NODE_TRY_STMT);
    // Expect colon
    if (parser->current.type != TOKEN_COLON) {
        fprintf(stderr, "Expected ':' after try\n");
        parser->had_error = true;
        return NULL;
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse try body
    node->as.try_stmt.try_body = parse_block(parser);
    // Expect catch
    if (parser->current.type == TOKEN_IDENTIFIER && strcmp(parser->current.lexeme, "catch") == 0) {
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        // Expect colon
        if (parser->current.type != TOKEN_COLON) {
            fprintf(stderr, "Expected ':' after catch\n");
            parser->had_error = true;
            return NULL;
        }
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        node->as.try_stmt.catch_body = parse_block(parser);
    } else {
        node->as.try_stmt.catch_body = NULL;
    }
    return node;
}

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
    // No semicolon required here; handled in parse_block
    
    ASTNode* node = make_node(NODE_FUNCTION_CALL);
    node->as.function.name = strdup("print");
    node->as.function.params = expr; // Always pass the full expression, do not unwrap function calls
    return node;
}

static ASTNode* parse_block(Parser* parser) {
    ASTNode* block = make_node(NODE_BLOCK);
    ASTNode* current = NULL;
    int is_curly = check(parser, TOKEN_LBRACE);
    
    // Skip the opening brace if present
    if (is_curly) {
        advance(parser);
    }
    
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF) && !check(parser, TOKEN_END)) {
        // Skip empty statements (standalone semicolons)
        if (parser->current.type == TOKEN_SEMICOLON) {
            advance(parser);
            continue;
        }
        
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            if (parser->had_error) {
                // On error, advance to next statement to prevent infinite loops
                while (!check(parser, TOKEN_EOF) && 
                       !check(parser, TOKEN_SEMICOLON) && 
                       !check(parser, TOKEN_RBRACE) && 
                       !check(parser, TOKEN_END)) {
                    advance(parser);
                }
                if (check(parser, TOKEN_SEMICOLON)) {
                    advance(parser);
                }
                continue;
            }
            // If we get a NULL statement without an error, it means we hit an 'end' token
            if (check(parser, TOKEN_END)) {
                advance(parser);  // Advance past the 'end' token
                break;
            }
            continue;
        }
        
        if (!block->as.block.statements) {
            block->as.block.statements = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
        
        // Only require semicolons in curly-brace blocks
        if (is_curly) {
            if (!match(parser, TOKEN_SEMICOLON)) {
                consume(parser, TOKEN_SEMICOLON, "Expect ';' after statement.");
            }
        }
    }
    
    // Always advance past block end token
    if (check(parser, TOKEN_RBRACE)) {
        advance(parser);
    } else if (check(parser, TOKEN_END)) {  // Use check() first to avoid consuming if not present
        advance(parser);  // Then advance past it
        fprintf(stderr, "DEBUG: Consumed TOKEN_END, now at token type %d, lexeme '%s', line %d\n", 
                parser->current.type, parser->current.lexeme, parser->current.line);
    } else {
        consume(parser, TOKEN_RBRACE, "Expect '}' or 'end' after block.");
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

static ASTNode* parse_return_stmt(Parser* parser) {
    ASTNode* node = make_node(NODE_RETURN_STMT);
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    // Parse the return expression if present
    if (parser->current.type != TOKEN_SEMICOLON) {
        node->as.unary.operand = parse_expression(parser);
    } else {
        node->as.unary.operand = NULL;
    }
    return node;
} 