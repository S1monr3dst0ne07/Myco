#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

#define MAX_CHILDREN 100

// Forward declarations
static ASTNode* parse_expression(Token* tokens, int* current);
static ASTNode* parse_statement(Token* tokens, int* current);
static ASTNode* parse_block(Token* tokens, int* current);

// Helper function to get operator precedence
static int get_precedence(const char* op) {
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) return 1;
    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || 
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) return 2;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 3;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 4;
    return 0;
}

// Helper function to parse primary expressions
static ASTNode* parse_primary(Token* tokens, int* current) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    // Set line number from current token
    node->line = tokens[*current].line;

    switch (tokens[*current].type) {
        case TOKEN_NUMBER:
        case TOKEN_IDENTIFIER:
            node->type = AST_EXPR;
            node->text = strdup(tokens[*current].text);
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++;
            break;

        case TOKEN_STRING:
            node->type = AST_EXPR;
            // Wrap the string literal in quotes
            char* quoted = (char*)malloc(strlen(tokens[*current].text) + 3);
            sprintf(quoted, "\"%s\"", tokens[*current].text);
            node->text = quoted;
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++;
            break;

        case TOKEN_LPAREN:
            (*current)++; // Skip '('
            node = parse_expression(tokens, current);
            if (!node) return NULL;
            if (tokens[*current].type != TOKEN_RPAREN) {
                fprintf(stderr, "Error: Expected ')' at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                return NULL;
            }
            (*current)++; // Skip ')'
            break;

        default:
            fprintf(stderr, "Error: Unexpected token in expression at line %d\n", tokens[*current].line);
            free(node);
            return NULL;
    }

    // Handle function calls
    if (tokens[*current].type == TOKEN_LPAREN) {
        (*current)++; // Skip '('
        ASTNode* call_node = (ASTNode*)malloc(sizeof(ASTNode));
        call_node->type = AST_EXPR;
        call_node->text = strdup("call");
        call_node->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
        call_node->child_count = 2;
        call_node->next = NULL;
        call_node->line = node->line;  // Preserve line number
        call_node->children[0] = *node; // Function name

        // Parse arguments
        ASTNode* args = (ASTNode*)malloc(sizeof(ASTNode));
        args->type = AST_EXPR;
        args->text = strdup("args");
        args->children = NULL;
        args->child_count = 0;
        args->next = NULL;
        args->line = tokens[*current].line;  // Set line number for args

        while (tokens[*current].type != TOKEN_RPAREN) {
            ASTNode* arg = parse_expression(tokens, current);
            if (!arg) {
                parser_free_ast(call_node);
                return NULL;
            }
            args->children = (ASTNode*)realloc(args->children, (args->child_count + 1) * sizeof(ASTNode));
            args->children[args->child_count++] = *arg;

            if (tokens[*current].type == TOKEN_COMMA) {
                (*current)++;
            } else if (tokens[*current].type != TOKEN_RPAREN) {
                fprintf(stderr, "Error: Expected ',' or ')' at line %d\n", tokens[*current].line);
                parser_free_ast(call_node);
                return NULL;
            }
        }
        (*current)++; // Skip ')'
        call_node->children[1] = *args;
        *node = *call_node;
    }

    return node;
}

// Helper function to parse expressions with operator precedence
static ASTNode* parse_expression(Token* tokens, int* current) {
    ASTNode* left = parse_primary(tokens, current);
    if (!left) {
        return NULL;
    }

    while (tokens[*current].type == TOKEN_OPERATOR) {
        char* op = tokens[*current].text;
        int op_prec = get_precedence(op);
        int op_line = tokens[*current].line;  // Store operator line number
        (*current)++;

        ASTNode* right = parse_primary(tokens, current);
        if (!right) {
            parser_free_ast(left);
            return NULL;
        }

        // Create operator node
        ASTNode* operator_node = (ASTNode*)malloc(sizeof(ASTNode));
        if (!operator_node) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            parser_free_ast(left);
            parser_free_ast(right);
            return NULL;
        }

        operator_node->type = AST_EXPR;
        operator_node->text = strdup(op);
        operator_node->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
        operator_node->child_count = 2;
        operator_node->next = NULL;
        operator_node->line = op_line;  // Set operator line number
        operator_node->children[0] = *left;
        operator_node->children[1] = *right;
        *left = *operator_node;

        // Check for higher precedence operators
        while (tokens[*current].type == TOKEN_OPERATOR) {
            char* next_op = tokens[*current].text;
            int next_prec = get_precedence(next_op);
            if (next_prec <= op_prec) {
                break;
            }

            int next_op_line = tokens[*current].line;  // Store next operator line number
            (*current)++;
            ASTNode* next_right = parse_primary(tokens, current);
            if (!next_right) {
                parser_free_ast(left);
                return NULL;
            }

            // Create new operator node for higher precedence operator
            ASTNode* next_operator = (ASTNode*)malloc(sizeof(ASTNode));
            if (!next_operator) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(left);
                parser_free_ast(next_right);
                return NULL;
            }

            next_operator->type = AST_EXPR;
            next_operator->text = strdup(next_op);
            next_operator->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
            next_operator->child_count = 2;
            next_operator->next = NULL;
            next_operator->line = next_op_line;  // Set next operator line number
            next_operator->children[0] = left->children[1]; // Right child of current operator
            next_operator->children[1] = *next_right;
            left->children[1] = *next_operator; // Update right child of current operator
        }
    }

    return left;
}

// Helper function to parse a block of statements
static ASTNode* parse_block(Token* tokens, int* current) {
    ASTNode* block = (ASTNode*)malloc(sizeof(ASTNode));
    if (!block) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    block->type = AST_BLOCK;
    block->text = strdup("block");
    block->children = NULL;
    block->child_count = 0;
    block->next = NULL;

    while (tokens[*current].type != TOKEN_END && 
           tokens[*current].type != TOKEN_ELSE && 
           tokens[*current].type != TOKEN_CATCH) {
        // Skip any semicolons at the start
        while (tokens[*current].type == TOKEN_SEMICOLON) {
            (*current)++;
        }

        // If we hit end, break
        if (tokens[*current].type == TOKEN_END) {
            break;
        }

        ASTNode* stmt = parse_statement(tokens, current);
        if (!stmt) {
            parser_free_ast(block);
            return NULL;
        }
        block->children = (ASTNode*)realloc(block->children, (block->child_count + 1) * sizeof(ASTNode));
        block->children[block->child_count++] = *stmt;

        // Skip semicolon if present
        if (tokens[*current].type == TOKEN_SEMICOLON) {
            (*current)++;
        }

        // If we hit end, break
        if (tokens[*current].type == TOKEN_END) {
            break;
        }
    }

    // Skip the end token if we found one
    if (tokens[*current].type == TOKEN_END) {
        (*current)++;
    }

    return block;
}

// Helper function to parse statements
static ASTNode* parse_statement(Token* tokens, int* current) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    // Handle 'use <path|string|identifier> as <identifier>'
    if (tokens[*current].type == TOKEN_USE) {
        int line = tokens[*current].line;
        (*current)++; // skip 'use'
        if (tokens[*current].type != TOKEN_PATH && tokens[*current].type != TOKEN_STRING && tokens[*current].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error: Expected module path or name after 'use' at line %d\n", tokens[*current].line);
            free(node); return NULL;
        }
        char* path = strdup(tokens[*current].text);
        (*current)++;
        if (tokens[*current].type != TOKEN_AS) {
            fprintf(stderr, "Error: Expected 'as' after module path at line %d\n", tokens[*current].line);
            free(node); free(path); return NULL;
        }
        (*current)++; // skip 'as'
        if (tokens[*current].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error: Expected identifier after 'as' at line %d\n", tokens[*current].line);
            free(node); free(path); return NULL;
        }
        char* alias = strdup(tokens[*current].text);
        (*current)++;
        if (tokens[*current].type == TOKEN_SEMICOLON) (*current)++;
        node->type = AST_BLOCK;
        node->text = strdup("use");
        node->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
        node->child_count = 2;
        node->next = NULL;
        node->line = line;
        node->children[0].type = AST_EXPR; node->children[0].text = path; node->children[0].children = NULL; node->children[0].child_count = 0; node->children[0].next = NULL; node->children[0].line = line;
        node->children[1].type = AST_EXPR; node->children[1].text = alias; node->children[1].children = NULL; node->children[1].child_count = 0; node->children[1].next = NULL; node->children[1].line = line;
        return node;
    }

    // First check if we're in a switch statement context
    if (tokens[*current].type == TOKEN_DEFAULT) {
        (*current)++; // Skip 'default'
        if (tokens[*current].type != TOKEN_COLON) {
            fprintf(stderr, "Error: Expected ':' after default at line %d\n", tokens[*current].line);
            free(node);
            return NULL;
        }
        (*current)++; // Skip ':'

        // Create default body node
        node->type = AST_DEFAULT;
        node->text = strdup("default");
        node->children = NULL;
        node->child_count = 0;
        node->next = NULL;

        // Parse the default case body as a block
        ASTNode* block = parse_block(tokens, current);
        if (!block) {
            parser_free_ast(node);
            return NULL;
        }

        // Add block as child of default node
        node->children = (ASTNode*)malloc(sizeof(ASTNode));
        node->children[0] = *block;
        node->child_count = 1;

        return node;
    }

    // Now handle the specific statement types
    switch (tokens[*current].type) {
        case TOKEN_IF: {
            node->type = AST_IF;
            node->text = strdup("if");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'if'

            // Parse condition
            ASTNode* condition = parse_expression(tokens, current);
            if (!condition) {
                parser_free_ast(node);
                return NULL;
            }

            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after if condition at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(condition);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse if body
            ASTNode* if_body = parse_block(tokens, current);
            if (!if_body) {
                parser_free_ast(node);
                parser_free_ast(condition);
                return NULL;
            }

            // Check for else clause
            ASTNode* else_body = NULL;
            if (tokens[*current].type == TOKEN_ELSE) {
                (*current)++; // Skip 'else'
                if (tokens[*current].type != TOKEN_COLON) {
                    fprintf(stderr, "Error: Expected ':' after else at line %d\n", tokens[*current].line);
                    parser_free_ast(node);
                    parser_free_ast(condition);
                    parser_free_ast(if_body);
                    return NULL;
                }
                (*current)++; // Skip ':'

                else_body = parse_block(tokens, current);
                if (!else_body) {
                    parser_free_ast(node);
                    parser_free_ast(condition);
                    parser_free_ast(if_body);
                    return NULL;
                }
            }

            // Add condition and bodies as children
            node->children = (ASTNode*)malloc((else_body ? 3 : 2) * sizeof(ASTNode));
            node->children[0] = *condition;
            node->children[1] = *if_body;
            if (else_body) {
                node->children[2] = *else_body;
                node->child_count = 3;
            } else {
                node->child_count = 2;
            }
            break;
        }

        case TOKEN_FOR: {
            node->type = AST_FOR;
            node->text = strdup("for");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'for'

            // Parse loop variable
            if (tokens[*current].type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Error: Expected loop variable at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                return NULL;
            }

            ASTNode* loop_var = (ASTNode*)malloc(sizeof(ASTNode));
            loop_var->type = AST_EXPR;
            loop_var->text = strdup(tokens[*current].text);
            loop_var->children = NULL;
            loop_var->child_count = 0;
            loop_var->next = NULL;
            (*current)++; // Skip loop variable

            // Parse 'in' keyword
            if (tokens[*current].type != TOKEN_IN) {
                fprintf(stderr, "Error: Expected 'in' keyword at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(loop_var);
                return NULL;
            }
            (*current)++; // Skip 'in'

            // Parse range start
            ASTNode* range_start = parse_expression(tokens, current);
            if (!range_start) {
                parser_free_ast(node);
                parser_free_ast(loop_var);
                return NULL;
            }

            // Parse range separator
            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(loop_var);
                parser_free_ast(range_start);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse range end
            ASTNode* range_end = parse_expression(tokens, current);
            if (!range_end) {
                parser_free_ast(node);
                parser_free_ast(loop_var);
                parser_free_ast(range_start);
                return NULL;
            }

            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(loop_var);
                parser_free_ast(range_start);
                parser_free_ast(range_end);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse loop body
            ASTNode* loop_body = parse_block(tokens, current);
            if (!loop_body) {
                parser_free_ast(node);
                parser_free_ast(loop_var);
                parser_free_ast(range_start);
                parser_free_ast(range_end);
                return NULL;
            }

            // Add loop variable, range, and body as children
            node->children = (ASTNode*)malloc(4 * sizeof(ASTNode));
            node->children[0] = *loop_var;
            node->children[1] = *range_start;
            node->children[2] = *range_end;
            node->children[3] = *loop_body;
            node->child_count = 4;
            break;
        }

        case TOKEN_SWITCH: {
            node->type = AST_SWITCH;
            node->text = strdup("switch");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'switch'

            // Parse switch expression
            ASTNode* switch_expr = parse_expression(tokens, current);
            if (!switch_expr) {
                parser_free_ast(node);
                return NULL;
            }

            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after switch expression at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(switch_expr);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse cases
            ASTNode* cases = (ASTNode*)malloc(sizeof(ASTNode));
            cases->type = AST_BLOCK;
            cases->text = strdup("cases");
            cases->children = NULL;
            cases->child_count = 0;
            cases->next = NULL;

            while (tokens[*current].type == TOKEN_CASE || tokens[*current].type == TOKEN_DEFAULT) {
                if (tokens[*current].type == TOKEN_CASE) {
                    (*current)++; // Skip 'case'
                    ASTNode* case_expr = parse_expression(tokens, current);
                    if (!case_expr) {
                        parser_free_ast(node);
                        parser_free_ast(switch_expr);
                        parser_free_ast(cases);
                        return NULL;
                    }

                    if (tokens[*current].type != TOKEN_COLON) {
                        fprintf(stderr, "Error: Expected ':' after case expression at line %d\n", tokens[*current].line);
                        parser_free_ast(node);
                        parser_free_ast(switch_expr);
                        parser_free_ast(cases);
                        parser_free_ast(case_expr);
                        return NULL;
                    }
                    (*current)++; // Skip ':'

                    ASTNode* case_body = parse_block(tokens, current);
                    if (!case_body) {
                        parser_free_ast(node);
                        parser_free_ast(switch_expr);
                        parser_free_ast(cases);
                        parser_free_ast(case_expr);
                        return NULL;
                    }

                    ASTNode* case_node = (ASTNode*)malloc(sizeof(ASTNode));
                    case_node->type = AST_CASE;
                    case_node->text = strdup("case");
                    case_node->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
                    case_node->children[0] = *case_expr;
                    case_node->children[1] = *case_body;
                    case_node->child_count = 2;
                    case_node->next = NULL;

                    cases->children = (ASTNode*)realloc(cases->children, (cases->child_count + 1) * sizeof(ASTNode));
                    cases->children[cases->child_count++] = *case_node;
                } else if (tokens[*current].type == TOKEN_DEFAULT) {
                    (*current)++; // Skip 'default'
                    if (tokens[*current].type != TOKEN_COLON) {
                        fprintf(stderr, "Error: Expected ':' after default at line %d\n", tokens[*current].line);
                        parser_free_ast(node);
                        parser_free_ast(switch_expr);
                        parser_free_ast(cases);
                        return NULL;
                    }
                    (*current)++; // Skip ':'

                    ASTNode* default_body = parse_block(tokens, current);
                    if (!default_body) {
                        parser_free_ast(node);
                        parser_free_ast(switch_expr);
                        parser_free_ast(cases);
                        return NULL;
                    }

                    ASTNode* default_node = (ASTNode*)malloc(sizeof(ASTNode));
                    default_node->type = AST_DEFAULT;
                    default_node->text = strdup("default");
                    default_node->children = (ASTNode*)malloc(sizeof(ASTNode));
                    default_node->children[0] = *default_body;
                    default_node->child_count = 1;
                    default_node->next = NULL;

                    cases->children = (ASTNode*)realloc(cases->children, (cases->child_count + 1) * sizeof(ASTNode));
                    cases->children[cases->child_count++] = *default_node;
                }
            }

            // Add switch expression and cases as children
            node->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
            node->children[0] = *switch_expr;
            node->children[1] = *cases;
            node->child_count = 2;
            break;
        }

        case TOKEN_TRY: {
            node->type = AST_TRY;
            node->text = strdup("try");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'try'

            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after try at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse try body
            ASTNode* try_body = parse_block(tokens, current);
            if (!try_body) {
                parser_free_ast(node);
                return NULL;
            }

            // Parse catch block
            if (tokens[*current].type != TOKEN_CATCH) {
                fprintf(stderr, "Error: Expected 'catch' at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(try_body);
                return NULL;
            }
            (*current)++; // Skip 'catch'

            // Parse error variable
            if (tokens[*current].type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Error: Expected error variable name at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(try_body);
                return NULL;
            }

            ASTNode* error_var = (ASTNode*)malloc(sizeof(ASTNode));
            error_var->type = AST_EXPR;
            error_var->text = strdup(tokens[*current].text);
            error_var->children = NULL;
            error_var->child_count = 0;
            error_var->next = NULL;
            (*current)++; // Skip error variable

            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after catch variable at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(try_body);
                parser_free_ast(error_var);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse catch body
            ASTNode* catch_body = parse_block(tokens, current);
            if (!catch_body) {
                parser_free_ast(node);
                parser_free_ast(try_body);
                parser_free_ast(error_var);
                return NULL;
            }

            // Add try body, error variable, and catch body as children
            node->children = (ASTNode*)malloc(3 * sizeof(ASTNode));
            node->children[0] = *try_body;
            node->children[1] = *error_var;
            node->children[2] = *catch_body;
            node->child_count = 3;
            break;
        }

        case TOKEN_LET: {
            node->type = AST_LET;
            node->text = strdup("let");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            node->line = tokens[*current].line;
            (*current)++; // Skip 'let'

            // Parse variable name
            if (tokens[*current].type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Error: Expected variable name at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                return NULL;
            }

            ASTNode* var_name = (ASTNode*)malloc(sizeof(ASTNode));
            var_name->type = AST_EXPR;
            var_name->text = strdup(tokens[*current].text);
            var_name->children = NULL;
            var_name->child_count = 0;
            var_name->next = NULL;
            (*current)++; // Skip variable name

            // Parse assignment operator
            if (tokens[*current].type != TOKEN_ASSIGN) {
                fprintf(stderr, "Error: Expected '=' at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(var_name);
                return NULL;
            }
            (*current)++; // Skip '='

            // Parse initial value
            ASTNode* init_value = parse_expression(tokens, current);
            if (!init_value) {
                parser_free_ast(node);
                parser_free_ast(var_name);
                return NULL;
            }

            if (tokens[*current].type != TOKEN_SEMICOLON) {
                fprintf(stderr, "Error: Expected ';' after variable declaration at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(var_name);
                parser_free_ast(init_value);
                return NULL;
            }
            (*current)++; // Skip ';'

            // Add variable name and initial value as children
            node->children = (ASTNode*)malloc(2 * sizeof(ASTNode));
            node->children[0] = *var_name;
            node->children[1] = *init_value;
            node->child_count = 2;
            break;
        }

        case TOKEN_RETURN: {
            node->type = AST_RETURN;
            node->text = strdup("return");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'return'

            // Parse return expression
            ASTNode* return_expr = parse_expression(tokens, current);
            if (!return_expr) {
                parser_free_ast(node);
                return NULL;
            }

            if (tokens[*current].type != TOKEN_SEMICOLON) {
                fprintf(stderr, "Error: Expected ';' after return statement at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(return_expr);
                return NULL;
            }
            (*current)++; // Skip ';'

            node->children = (ASTNode*)malloc(sizeof(ASTNode));
            node->children[0] = *return_expr;
            node->child_count = 1;
            break;
        }

        case TOKEN_PRINT: {
            node->type = AST_PRINT;
            node->text = strdup("print");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'print'

            // Parse print arguments
            if (tokens[*current].type != TOKEN_LPAREN) {
                fprintf(stderr, "Error: Expected '(' after print at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                return NULL;
            }
            (*current)++; // Skip '('

            while (tokens[*current].type != TOKEN_RPAREN) {
                ASTNode* arg = parse_expression(tokens, current);
                if (!arg) {
                    parser_free_ast(node);
                    return NULL;
                }

                node->children = (ASTNode*)realloc(node->children, (node->child_count + 1) * sizeof(ASTNode));
                node->children[node->child_count++] = *arg;

                if (tokens[*current].type == TOKEN_COMMA) {
                    (*current)++; // Skip ','
                } else if (tokens[*current].type != TOKEN_RPAREN) {
                    fprintf(stderr, "Error: Expected ',' or ')' at line %d\n", tokens[*current].line);
                    parser_free_ast(node);
                    return NULL;
                }
            }
            (*current)++; // Skip ')'

            if (tokens[*current].type != TOKEN_SEMICOLON) {
                fprintf(stderr, "Error: Expected ';' after print statement at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                return NULL;
            }
            (*current)++; // Skip ';'
            break;
        }

        default:
            fprintf(stderr, "Error: Unexpected token in statement at line %d\n", tokens[*current].line);
            parser_free_ast(node);
            return NULL;
    }

    return node;
}

ASTNode* parser_parse(Token* tokens) {
    int current = 0;
    ASTNode* root = (ASTNode*)malloc(sizeof(ASTNode));
    if (!root) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    root->type = AST_BLOCK;
    root->text = strdup("block");
    root->children = NULL;
    root->child_count = 0;
    root->next = NULL;

    while (tokens[current].type != TOKEN_EOF) {
        ASTNode* node = NULL;

        // Parse function declarations
        if (tokens[current].type == TOKEN_FUNC) {
            current++; // Skip 'func'
            
            if (tokens[current].type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Error: Expected function name at line %d\n", tokens[current].line);
                parser_free_ast(root);
                return NULL;
            }
            
            node = (ASTNode*)malloc(sizeof(ASTNode));
            if (!node) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(root);
                return NULL;
            }
            node->type = AST_FUNC;
            node->text = strdup(tokens[current].text);
            if (!node->text) {
                fprintf(stderr, "Error: Memory allocation failed for function name\n");
                parser_free_ast(root);
                free(node);
                return NULL;
            }
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            current++; // Skip function name

            // Parse parameters
            if (tokens[current].type != TOKEN_LPAREN) {
                fprintf(stderr, "Error: Expected '(' at line %d\n", tokens[current].line);
                parser_free_ast(root);
                parser_free_ast(node);
                return NULL;
            }
            current++; // Skip '('

            while (tokens[current].type != TOKEN_RPAREN) {
                if (tokens[current].type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Error: Expected parameter name at line %d\n", tokens[current].line);
                    parser_free_ast(root);
                    parser_free_ast(node);
                    return NULL;
                }

                ASTNode* param = (ASTNode*)malloc(sizeof(ASTNode));
                if (!param) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    parser_free_ast(root);
                    parser_free_ast(node);
                    return NULL;
                }
                param->type = AST_EXPR;
                param->text = strdup(tokens[current].text);
                if (!param->text) {
                    fprintf(stderr, "Error: Memory allocation failed for parameter name\n");
                    parser_free_ast(root);
                    parser_free_ast(node);
                    free(param);
                    return NULL;
                }
                param->children = NULL;
                param->child_count = 0;
                param->next = NULL;
                current++; // Skip parameter name

                // Parse type annotation
                if (tokens[current].type == TOKEN_COLON) {
                    current++; // Skip ':'
                    if (tokens[current].type != TOKEN_TYPE) {
                        fprintf(stderr, "Error: Expected type annotation at line %d\n", tokens[current].line);
                        parser_free_ast(root);
                        parser_free_ast(node);
                        parser_free_ast(param);
                        return NULL;
                    }
                    ASTNode* type = (ASTNode*)malloc(sizeof(ASTNode));
                    if (!type) {
                        fprintf(stderr, "Error: Memory allocation failed\n");
                        parser_free_ast(root);
                        parser_free_ast(node);
                        parser_free_ast(param);
                        return NULL;
                    }
                    type->type = AST_EXPR;
                    type->text = strdup(tokens[current].text);
                    if (!type->text) {
                        fprintf(stderr, "Error: Memory allocation failed for type name\n");
                        parser_free_ast(root);
                        parser_free_ast(node);
                        parser_free_ast(param);
                        free(type);
                        return NULL;
                    }
                    type->children = NULL;
                    type->child_count = 0;
                    type->next = NULL;
                    current++; // Skip type

                    // Add type as child of parameter
                    param->children = (ASTNode*)malloc(sizeof(ASTNode));
                    param->children[0] = *type;
                    param->child_count = 1;
                }

                // Add parameter as child of function
                node->children = (ASTNode*)realloc(node->children, (node->child_count + 1) * sizeof(ASTNode));
                node->children[node->child_count++] = *param;

                if (tokens[current].type == TOKEN_COMMA) {
                    current++; // Skip ','
                } else if (tokens[current].type != TOKEN_RPAREN) {
                    fprintf(stderr, "Error: Expected ',' or ')' at line %d\n", tokens[current].line);
                    parser_free_ast(root);
                    parser_free_ast(node);
                    return NULL;
                }
            }
            current++; // Skip ')'

            // Parse return type
            if (tokens[current].type == TOKEN_COLON) {
                current++; // Skip ':'
                if (tokens[current].type != TOKEN_TYPE) {
                    fprintf(stderr, "Error: Expected return type at line %d\n", tokens[current].line);
                    parser_free_ast(root);
                    parser_free_ast(node);
                    return NULL;
                }
                ASTNode* return_type = (ASTNode*)malloc(sizeof(ASTNode));
                if (!return_type) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    parser_free_ast(root);
                    parser_free_ast(node);
                    return NULL;
                }
                return_type->type = AST_EXPR;
                return_type->text = strdup(tokens[current].text);
                if (!return_type->text) {
                    fprintf(stderr, "Error: Memory allocation failed for return type\n");
                    parser_free_ast(root);
                    parser_free_ast(node);
                    free(return_type);
                    return NULL;
                }
                return_type->children = NULL;
                return_type->child_count = 0;
                return_type->next = NULL;
                current++; // Skip return type

                // Add return type as child of function
                node->children = (ASTNode*)realloc(node->children, (node->child_count + 1) * sizeof(ASTNode));
                node->children[node->child_count++] = *return_type;
            }

            // Parse function body
            if (tokens[current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after function declaration at line %d\n", tokens[current].line);
                parser_free_ast(root);
                parser_free_ast(node);
                return NULL;
            }
            current++; // Skip ':'

            ASTNode* body = parse_block(tokens, &current);
            if (!body) {
                parser_free_ast(root);
                parser_free_ast(node);
                return NULL;
            }

            // Add body as child of function
            node->children = (ASTNode*)realloc(node->children, (node->child_count + 1) * sizeof(ASTNode));
            if (!node->children) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(root);
                parser_free_ast(node);
                parser_free_ast(body);
                return NULL;
            }
            node->children[node->child_count++] = *body;
        }
        // Parse other statements
        else {
            node = parse_statement(tokens, &current);
            if (!node) {
                parser_free_ast(root);
                return NULL;
            }
        }

        if (node) {
            root->children = (ASTNode*)realloc(root->children, (root->child_count + 1) * sizeof(ASTNode));
            if (!root->children) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(root);
                parser_free_ast(node);
                return NULL;
            }
            root->children[root->child_count++] = *node;
        }
    }

    return root;
}

void parser_free_ast(ASTNode* node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        parser_free_ast(&node->children[i]);
    }
    free(node->children);
    free(node->text);
    free(node);
} 