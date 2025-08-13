#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"
#include "memory_tracker.h"

#define MAX_CHILDREN 100

// Forward declarations
static ASTNode* parse_expression(Token* tokens, int* current);
static ASTNode* parse_statement(Token* tokens, int* current);
static ASTNode* parse_block(Token* tokens, int* current);
static void deep_copy_ast_node(ASTNode* dest, ASTNode* src);

// Helper function to get operator precedence
static int get_precedence(const char* op) {
    if (strcmp(op, "and") == 0 || strcmp(op, "or") == 0) return 1;
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) return 2;
    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || 
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) return 3;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 4;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 5;
    return 0;
}

// Helper function to parse primary expressions
static ASTNode* parse_primary(Token* tokens, int* current) {
    ASTNode* node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_primary");
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

        case TOKEN_OPERATOR:
            // Handle unary minus (negative numbers)
            if (strcmp(tokens[*current].text, "-") == 0) {
                (*current)++; // Skip '-'
                
                // Parse the number after the minus
                if (tokens[*current].type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected number after '-' at line %d\n", tokens[*current].line);
                    tracked_free(node, __FILE__, __LINE__, "parse_primary_unary_minus");
                    return NULL;
                }
                
                // Create a negative number by combining '-' and the number
                char* negative_num = (char*)malloc(strlen(tokens[*current].text) + 2);
                sprintf(negative_num, "-%s", tokens[*current].text);
                
                node->type = AST_EXPR;
                node->text = negative_num;
                node->children = NULL;
                node->child_count = 0;
                node->next = NULL;
                (*current)++;
                break;
            } else {
                fprintf(stderr, "Error: Unexpected operator '%s' in expression at line %d\n", 
                        tokens[*current].text, tokens[*current].line);
                tracked_free(node, __FILE__, __LINE__, "parse_primary_unexpected_operator");
                return NULL;
            }

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
                tracked_free(node, __FILE__, __LINE__, "parse_primary");
                return NULL;
            }
            (*current)++; // Skip ')'
            break;

        default:
            fprintf(stderr, "Error: Unexpected token '%s' in expression at line %d (token type: %d)\n", 
                    tokens[*current].text ? tokens[*current].text : "NULL", 
                    tokens[*current].line, 
                    tokens[*current].type);
            tracked_free(node, __FILE__, __LINE__, "parse_primary");
            return NULL;
    }

    // Handle dot expressions (member access) - do this BEFORE function calls
    while (tokens[*current].type == TOKEN_DOT) {
        (*current)++; // Skip '.'
        
        if (tokens[*current].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error: Expected identifier after '.' at line %d, got token type %d\n", 
                    tokens[*current].line, tokens[*current].type);
            parser_free_ast(node);
            return NULL;
        }

        ASTNode* dot_node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_primary_dot");
        dot_node->type = AST_DOT;
        dot_node->text = strdup("dot");
        dot_node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_primary_dot");
        dot_node->child_count = 2;
        dot_node->next = NULL;
        dot_node->line = node->line;

        // Left side (object) - move the node structure instead of copying
        dot_node->children[0] = *node;
        // Clear the original node's pointers to prevent double-free
        node->text = NULL;
        node->children = NULL;
        node->child_count = 0;

        // Right side (member name)
        dot_node->children[1].type = AST_EXPR;
        dot_node->children[1].text = strdup(tokens[*current].text);
        dot_node->children[1].children = NULL;
        dot_node->children[1].child_count = 0;
        dot_node->children[1].next = NULL;
        dot_node->children[1].line = tokens[*current].line;

        (*current)++; // Skip member identifier
        

        
        // Free the original node since we're replacing it
        tracked_free(node, __FILE__, __LINE__, "parse_primary_dot_replace");
        node = dot_node;
    }

    // Handle function calls - do this AFTER dot expressions
    if (tokens[*current].type == TOKEN_LPAREN) {
        (*current)++; // Skip '('
        ASTNode* call_node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_primary_call");
        call_node->type = AST_EXPR;
        call_node->text = strdup("call");
        call_node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_primary_call");
        call_node->child_count = 2;
        call_node->next = NULL;
        call_node->line = node->line;  // Preserve line number
        
        // Use deep copy to properly copy the function name node
        deep_copy_ast_node(&call_node->children[0], node);

        // Parse arguments
        call_node->children[1].type = AST_EXPR;
        call_node->children[1].text = strdup("args");
        call_node->children[1].children = NULL;
        call_node->children[1].child_count = 0;
        call_node->children[1].next = NULL;
        call_node->children[1].line = tokens[*current].line;  // Set line number for args

        while (tokens[*current].type != TOKEN_RPAREN) {
            ASTNode* arg = parse_expression(tokens, current);
            if (!arg) {
                tracked_free(call_node, __FILE__, __LINE__, "parse_primary_call_error");
                return NULL;
            }
            call_node->children[1].children = (ASTNode*)tracked_realloc(call_node->children[1].children, (call_node->children[1].child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_primary_call_args");
            // Use deep copy to properly copy the argument node
            deep_copy_ast_node(&call_node->children[1].children[call_node->children[1].child_count], arg);
            call_node->children[1].child_count++;
            tracked_free(arg, __FILE__, __LINE__, "parse_primary_call_arg");

            if (tokens[*current].type == TOKEN_COMMA) {
                (*current)++;
            } else if (tokens[*current].type != TOKEN_RPAREN) {
                fprintf(stderr, "Error: Expected ',' or ')' at line %d\n", tokens[*current].line);
                tracked_free(call_node, __FILE__, __LINE__, "parse_primary_call_syntax_error");
                return NULL;
            }
        }
        (*current)++; // Skip ')'
        
        // Free the original node since we're replacing it
        tracked_free(node, __FILE__, __LINE__, "parse_primary_call_replace");
        node = call_node;
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
        ASTNode* operator_node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_expression_operator");
        if (!operator_node) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            parser_free_ast(left);
            parser_free_ast(right);
            return NULL;
        }

        operator_node->type = AST_EXPR;
        operator_node->text = strdup(op);
        operator_node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_expression_operator");
        operator_node->child_count = 2;
        operator_node->next = NULL;
        operator_node->line = op_line;  // Set operator line number
        

        
        // Move the left and right nodes directly instead of deep copying
        operator_node->children[0] = *left;
        operator_node->children[1] = *right;
        
        // Free the original nodes since we've moved their content
        tracked_free(left, __FILE__, __LINE__, "parse_expression_operator_move");
        tracked_free(right, __FILE__, __LINE__, "parse_expression_operator_move");
        
        left = operator_node;

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
            ASTNode* next_operator = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_expression_next_operator");
            if (!next_operator) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(left);
                parser_free_ast(next_right);
                return NULL;
            }

            next_operator->type = AST_EXPR;
            next_operator->text = strdup(next_op);
            next_operator->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_expression_next_operator");
            next_operator->child_count = 2;
            next_operator->next = NULL;
            next_operator->line = next_op_line;  // Set next operator line number
            
            // Move the nodes directly instead of deep copying
            next_operator->children[0] = left->children[1];
            next_operator->children[1] = *next_right;
            
            // Free the new right node since we've moved its content
            tracked_free(next_right, __FILE__, __LINE__, "parse_expression_next_operator_move");
            
            // Update the right child of current operator by moving
            left->children[1] = *next_operator;
            
            // Free the next operator node since we've moved its content
            tracked_free(next_operator, __FILE__, __LINE__, "parse_expression_next_operator_move");
        }
    }

    return left;
}

// Helper function to parse a block of statements
static ASTNode* parse_block(Token* tokens, int* current) {
    ASTNode* block = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_block");
    if (!block) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    block->type = AST_BLOCK;
    block->text = strdup("block");
    block->children = NULL;
    block->child_count = 0;
    block->next = NULL;
    block->for_type = AST_FOR_RANGE;  // Initialize for_type field

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
        
        // Allocate space for the new statement
        block->children = (ASTNode*)tracked_realloc(block->children, (block->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_block");
        
        // Move the statement directly instead of deep copying
        block->children[block->child_count] = *stmt;
        
        // Deep copy the text to prevent corruption
        if (stmt->text) {
            block->children[block->child_count].text = strdup(stmt->text);
        }
        
        // Deep copy all children to prevent corruption
        if (stmt->children && stmt->child_count > 0) {
            block->children[block->child_count].children = (ASTNode*)tracked_malloc(stmt->child_count * sizeof(ASTNode), __FILE__, __LINE__, "parse_block");
            block->children[block->child_count].child_count = stmt->child_count;
            
            for (int j = 0; j < stmt->child_count; j++) {
                // Copy the child node
                block->children[block->child_count].children[j] = stmt->children[j];
                
                // Deep copy the child's text
                if (stmt->children[j].text) {
                    block->children[block->child_count].children[j].text = strdup(stmt->children[j].text);
                }
                
                // Deep copy the child's children recursively
                if (stmt->children[j].children && stmt->children[j].child_count > 0) {
                    block->children[block->child_count].children[j].children = (ASTNode*)tracked_malloc(stmt->children[j].child_count * sizeof(ASTNode), __FILE__, __LINE__, "parse_block");
                    block->children[block->child_count].children[j].child_count = stmt->children[j].child_count;
                    
                    for (int k = 0; k < stmt->children[j].child_count; k++) {
                        block->children[block->child_count].children[j].children[k] = stmt->children[j].children[k];
                        
                        if (stmt->children[j].children[k].text) {
                            block->children[block->child_count].children[j].children[k].text = strdup(stmt->children[j].children[k].text);
                        }
                    }
                }
            }
        }
        
        block->child_count++;
        
        // Clean up the source statement using tracked_free
        if (stmt->text) tracked_free(stmt->text, __FILE__, __LINE__, "parse_block");
        if (stmt->children) tracked_free(stmt->children, __FILE__, __LINE__, "parse_block");
        tracked_free(stmt, __FILE__, __LINE__, "parse_block");

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

// Helper function to safely copy an AST node (shallow copy)
// Recursive deep copy function for AST nodes
static void deep_copy_ast_node(ASTNode* dest, ASTNode* src) {
    dest->type = src->type;
    dest->line = src->line;
    dest->next = NULL;
    
    // Deep copy text
    if (src->text) {
        dest->text = strdup(src->text);
    } else {
        dest->text = NULL;
    }
    
    // Deep copy children recursively
    if (src->children && src->child_count > 0) {
        dest->children = (ASTNode*)tracked_malloc(src->child_count * sizeof(ASTNode), __FILE__, __LINE__, "deep_copy_ast_node");
        dest->child_count = src->child_count;
        
        for (int i = 0; i < src->child_count; i++) {
            deep_copy_ast_node(&dest->children[i], &src->children[i]);
        }
    } else {
        dest->children = NULL;
        dest->child_count = 0;
    }
}

// Helper function to parse statements
static ASTNode* parse_statement(Token* tokens, int* current) {
    ASTNode* node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement");
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    // Initialize for_type field
    node->for_type = AST_FOR_RANGE;


    
    // Handle 'use <path|string|identifier> as <identifier>'
    if (tokens[*current].type == TOKEN_USE) {
        int line = tokens[*current].line;
        (*current)++; // skip 'use'
        if (tokens[*current].type != TOKEN_PATH && tokens[*current].type != TOKEN_STRING && tokens[*current].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error: Expected module path or name after 'use' at line %d\n", tokens[*current].line);
            tracked_free(node, __FILE__, __LINE__, "parse_statement_use_error");
            return NULL;
        }
        char* path = strdup(tokens[*current].text);
        (*current)++;
        if (tokens[*current].type != TOKEN_AS) {
            fprintf(stderr, "Error: Expected 'as' after module path at line %d\n", tokens[*current].line);
            tracked_free(node, __FILE__, __LINE__, "parse_statement_use_as_error");
            free(path);
            return NULL;
        }
        (*current)++; // skip 'as'
        if (tokens[*current].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error: Expected identifier after 'as' at line %d\n", tokens[*current].line);
            tracked_free(node, __FILE__, __LINE__, "parse_statement_use_alias_error");
            free(path);
            return NULL;
        }
        char* alias = strdup(tokens[*current].text);
        (*current)++;
        if (tokens[*current].type == TOKEN_SEMICOLON) (*current)++;
        node->type = AST_BLOCK;
        node->text = strdup("use");
        node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_use");
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
        node->children = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_default");
        deep_copy_ast_node(&node->children[0], block);
        node->child_count = 1;

        return node;
    }

    // Now handle the specific statement types
    switch (tokens[*current].type) {
        case TOKEN_WHILE: {
            node->type = AST_WHILE;
            node->text = strdup("while");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'while'

            // Parse condition
            ASTNode* condition = parse_expression(tokens, current);
            if (!condition) {
                parser_free_ast(node);
                return NULL;
            }

            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after while condition at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                parser_free_ast(condition);
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse while body
            ASTNode* while_body = parse_block(tokens, current);
            if (!while_body) {
                parser_free_ast(node);
                parser_free_ast(condition);
                return NULL;
            }

            // Add condition and body as children
            node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_while");
            node->children[0] = *condition;
            node->children[1] = *while_body;
            node->child_count = 2;
            break;
        }
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
            node->children = (ASTNode*)tracked_malloc((else_body ? 3 : 2) * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_if");
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
            node->for_type = AST_FOR_RANGE;  // Default to range loop
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

            ASTNode* loop_var = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_for");
            if (!loop_var) {
                parser_free_ast(node);
                return NULL;
            }
            
            loop_var->type = AST_EXPR;
            loop_var->text = strdup(tokens[*current].text);
            loop_var->children = NULL;
            loop_var->child_count = 0;
            loop_var->next = NULL;
            loop_var->for_type = AST_FOR_RANGE;  // Not used for this node type
            (*current)++; // Skip loop variable

            // Parse 'in' keyword
            if (tokens[*current].type != TOKEN_IN) {
                fprintf(stderr, "Error: Expected 'in' keyword at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }
            (*current)++; // Skip 'in'

            // Parse range start
            ASTNode* range_start = parse_expression(tokens, current);
            if (!range_start) {
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }

            // Parse range separator (first colon)
            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after range start at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }
            (*current)++; // Skip ':'

            // Parse range end
            ASTNode* range_end = parse_expression(tokens, current);
            if (!range_end) {
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }

            // Check for optional step (second colon) - but only if we're not at the final colon
            ASTNode* step = NULL;
            
            // A step separator is a colon followed by a number (not by a statement)
            // Handle both positive numbers (TOKEN_NUMBER) and negative numbers (TOKEN_OPERATOR + TOKEN_NUMBER)
            if (tokens[*current].type == TOKEN_COLON && 
                (tokens[*current + 1].type == TOKEN_NUMBER || 
                 (tokens[*current + 1].type == TOKEN_OPERATOR && tokens[*current + 1].text[0] == '-' && tokens[*current + 2].type == TOKEN_NUMBER))) {
                (*current)++; // Skip second ':'
                step = parse_expression(tokens, current);
                if (!step) {
                    parser_free_ast(node);
                    tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                    tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
                    tracked_free(range_end, __FILE__, __LINE__, "parse_statement_for");
                    return NULL;
                }
                node->for_type = AST_FOR_STEP;  // This is a step loop
            }

            // Parse final colon (required for all for loops)
            if (tokens[*current].type != TOKEN_COLON) {
                fprintf(stderr, "Error: Expected ':' after range end at line %d\n", tokens[*current].line);
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_end, __FILE__, __LINE__, "parse_statement_for");
                if (step) tracked_free(step, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }
            (*current)++; // Skip final ':'

            // Parse loop body
            ASTNode* loop_body = parse_block(tokens, current);
            if (!loop_body) {
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_end, __FILE__, __LINE__, "parse_statement_for");
                if (step) tracked_free(step, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }

            // Allocate children array based on whether we have a step
            int child_count = step ? 5 : 4;
            node->children = (ASTNode*)tracked_malloc(child_count * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_for");
            if (!node->children) {
                parser_free_ast(node);
                tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(range_end, __FILE__, __LINE__, "parse_statement_for");
                if (step) tracked_free(step, __FILE__, __LINE__, "parse_statement_for");
                tracked_free(loop_body, __FILE__, __LINE__, "parse_statement_for");
                return NULL;
            }

            // Add children: [loop_var, start, end, step?, body]
            node->children[0] = *loop_var;
            node->children[1] = *range_start;
            node->children[2] = *range_end;
            if (step) {
                node->children[3] = *step;
                node->children[4] = *loop_body;
            } else {
                node->children[3] = *loop_body;
            }
            node->child_count = child_count;

            // Clean up individual nodes (they're now copied to children array)
            tracked_free(loop_var, __FILE__, __LINE__, "parse_statement_for");
            tracked_free(range_start, __FILE__, __LINE__, "parse_statement_for");
            tracked_free(range_end, __FILE__, __LINE__, "parse_statement_for");
            if (step) tracked_free(step, __FILE__, __LINE__, "parse_statement_for");
            tracked_free(loop_body, __FILE__, __LINE__, "parse_statement_for");
            
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
            ASTNode* cases = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_switch");
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

                    ASTNode* case_node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_switch_case");
                    case_node->type = AST_CASE;
                    case_node->text = strdup("case");
                    case_node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_switch_case");
                    case_node->children[0] = *case_expr;
                    case_node->children[1] = *case_body;
                    case_node->child_count = 2;
                    case_node->next = NULL;

                    cases->children = (ASTNode*)tracked_realloc(cases->children, (cases->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_switch_case");
                    cases->children[cases->child_count] = *case_node;
                    cases->child_count++;
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

                    ASTNode* default_node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_switch_default");
                    default_node->type = AST_DEFAULT;
                    default_node->text = strdup("default");
                    default_node->children = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_switch_default");
                    default_node->children[0] = *default_body;
                    default_node->child_count = 1;
                    default_node->next = NULL;

                    cases->children = (ASTNode*)tracked_realloc(cases->children, (cases->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_switch_default");
                    cases->children[cases->child_count] = *default_node;
                    cases->child_count++;
                }
            }

            // Add switch expression and cases as children
            node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_switch");
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

            ASTNode* error_var = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_try");
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
            node->children = (ASTNode*)tracked_malloc(3 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_try");
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

            ASTNode* var_name = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_let");
            var_name->type = AST_EXPR;
            var_name->text = strdup(tokens[*current].text);
            var_name->children = NULL;
            var_name->child_count = 0;
            var_name->next = NULL;
            (*current)++; // Skip variable name

            // Check if this is a function definition (has parentheses) or variable assignment (has =)
            if (tokens[*current].type == TOKEN_LPAREN) {
                // This is a function definition
                node->type = AST_FUNC;
                node->text = strdup(var_name->text);
                free(var_name->text);
                free(var_name);
                
                // Parse parameters
                (*current)++; // Skip '('
                while (tokens[*current].type != TOKEN_RPAREN) {
                    if (tokens[*current].type != TOKEN_IDENTIFIER) {
                        fprintf(stderr, "Error: Expected parameter name at line %d\n", tokens[*current].line);
                        parser_free_ast(node);
                        return NULL;
                    }

                    ASTNode* param = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_func_param");
                    if (!param) {
                        fprintf(stderr, "Error: Memory allocation failed\n");
                        parser_free_ast(node);
                        return NULL;
                    }
                    param->type = AST_EXPR;
                    param->text = strdup(tokens[*current].text);
                    param->children = NULL;
                    param->child_count = 0;
                    param->next = NULL;
                    (*current)++; // Skip parameter name

                    // Parse type annotation if present
                    if (tokens[*current].type == TOKEN_COLON) {
                        (*current)++; // Skip ':'
                        if (tokens[*current].type != TOKEN_TYPE_MARKER && tokens[*current].type != TOKEN_STRING_TYPE) {
                            fprintf(stderr, "Error: Expected type annotation at line %d\n", tokens[*current].line);
                            parser_free_ast(node);
                            parser_free_ast(param);
                            return NULL;
                        }
                        (*current)++; // Skip type
                    }

                    // Add parameter to function
                    node->children = (ASTNode*)tracked_realloc(node->children, (node->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_func_param");
                    deep_copy_ast_node(&node->children[node->child_count], param);
                    node->child_count++;

                    // Skip comma if present
                    if (tokens[*current].type == TOKEN_COMMA) {
                        (*current)++;
                    }
                }
                (*current)++; // Skip ')'

                // Parse function body
                if (tokens[*current].type != TOKEN_COLON) {
                    fprintf(stderr, "Error: Expected ':' after function parameters at line %d\n", tokens[*current].line);
                    parser_free_ast(node);
                    return NULL;
                }
                (*current)++; // Skip ':'

                ASTNode* func_body = parse_block(tokens, current);
                if (!func_body) {
                    parser_free_ast(node);
                    return NULL;
                }

                // Add function body as child
                node->children = (ASTNode*)tracked_realloc(node->children, (node->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_func_body");
                deep_copy_ast_node(&node->children[node->child_count], func_body);
                node->child_count++;
            } else {
                // This is a variable assignment
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
                    parser_free_ast(node);
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
                node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_let_var");
                node->children[0] = *var_name;
                node->children[1] = *init_value;
                node->child_count = 2;
            }
            break;
        }

        case TOKEN_RETURN: {
            node->type = AST_RETURN;
            node->text = strdup("return");
            node->children = NULL;
            node->child_count = 0;
            node->next = NULL;
            (*current)++; // Skip 'return'

            // Check if there's a return expression or just a semicolon
            if (tokens[*current].type == TOKEN_SEMICOLON) {
                // No return value, just return;
                (*current)++; // Skip ';'
                node->child_count = 0;
            } else {
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

                node->children = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_return");
                node->children[0] = *return_expr;
                node->child_count = 1;
            }
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
                
                node->children = (ASTNode*)tracked_realloc(node->children, (node->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_print");
                // Move the arg node directly instead of deep copying
                node->children[node->child_count] = *arg;
                
                // Deep copy the text to prevent corruption
                if (arg->text) {
                    node->children[node->child_count].text = strdup(arg->text);
                }
                
                node->child_count++;

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

        case TOKEN_IDENTIFIER: {
            // Check if this is an assignment statement (identifier = expression)
            if (tokens[*current + 1].type == TOKEN_ASSIGN) {
                // This is an assignment statement
                char* var_name = strdup(tokens[*current].text);
                (*current)++; // Skip identifier
                (*current)++; // Skip '='
                
                // Parse the value expression
                ASTNode* value_expr = parse_expression(tokens, current);
                if (!value_expr) {
                    free(var_name);
                    parser_free_ast(node);
                    return NULL;
                }
                
                // Check for semicolon
                if (tokens[*current].type != TOKEN_SEMICOLON) {
                    fprintf(stderr, "Error: Expected ';' after assignment at line %d\n", tokens[*current].line);
                    free(var_name);
                    parser_free_ast(node);
                    parser_free_ast(value_expr);
                    return NULL;
                }
                (*current)++; // Skip ';'
                
                // Create assignment node
                node->type = AST_ASSIGN;
                node->text = strdup("assign");
                node->children = (ASTNode*)tracked_malloc(2 * sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_assignment");
                node->child_count = 2;
                node->next = NULL;
                node->line = tokens[*current - 2].line; // Line of the identifier
                
                // Set variable name and value
                node->children[0].type = AST_EXPR;
                node->children[0].text = var_name;
                node->children[0].children = NULL;
                node->children[0].child_count = 0;
                node->children[0].next = NULL;
                node->children[0].line = node->line;
                
                // Copy the value expression
                deep_copy_ast_node(&node->children[1], value_expr);
                parser_free_ast(value_expr);
                break;
            }
            
            // This might be a function call statement like "bot.send_embed(...)"
            // Save the current position to restore if this isn't a function call
            int saved_current = *current;
            
            // Try to parse as an expression (which will handle dot expressions and function calls)
            ASTNode* expr = parse_expression(tokens, current);
            if (!expr) {
                // Restore position and fall through to default
                *current = saved_current;
                goto default_case;
            }
            
            // Check if the expression ends with a semicolon (indicating it's a statement)
            if (tokens[*current].type == TOKEN_SEMICOLON) {
                (*current)++; // Skip semicolon
                
                // Create an expression statement node
                node->type = AST_EXPR;
                node->text = strdup("expr_stmt");
                node->children = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parse_statement_identifier");
                node->child_count = 1;
                node->next = NULL;
                node->line = tokens[saved_current].line;
                node->children[0] = *expr;
                break;
            } else {
                // Not a statement, restore position and fall through to default
                *current = saved_current;
                goto default_case;
            }
        }
        
        default_case:
        default:
            fprintf(stderr, "Error: Unexpected token in statement at line %d\n", tokens[*current].line);
            parser_free_ast(node);
            return NULL;
    }

    return node;
}

ASTNode* parser_parse(Token* tokens) {
    int current = 0;
    ASTNode* root = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parser_parse");
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
            
            node = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func");
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

                ASTNode* param = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func_param");
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
                    if (tokens[current].type != TOKEN_TYPE_MARKER && tokens[current].type != TOKEN_STRING_TYPE) {
                        fprintf(stderr, "Error: Expected type annotation at line %d\n", tokens[current].line);
                        parser_free_ast(root);
                        parser_free_ast(node);
                        parser_free_ast(param);
                        return NULL;
                    }
                    ASTNode* type = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func_type");
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
                    param->children = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func_param_type");
                    param->children[0] = *type;
                    param->child_count = 1;
                }

                // Add parameter as child of function
                node->children = (ASTNode*)tracked_realloc(node->children, (node->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func_param");
                node->children[node->child_count] = *param;
                node->child_count++;

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
                if (tokens[current].type != TOKEN_TYPE_MARKER && tokens[current].type != TOKEN_STRING_TYPE) {
                    fprintf(stderr, "Error: Expected return type at line %d\n", tokens[current].line);
                    parser_free_ast(root);
                    parser_free_ast(node);
                    return NULL;
                }
                ASTNode* return_type = (ASTNode*)tracked_malloc(sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func_return_type");
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
                node->children = (ASTNode*)tracked_realloc(node->children, (node->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_return_type");
                node->children[node->child_count] = *return_type;
                node->child_count++;
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
            node->children = (ASTNode*)tracked_realloc(node->children, (node->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_func_body");
            if (!node->children) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(root);
                parser_free_ast(node);
                parser_free_ast(body);
                return NULL;
            }
            node->children[node->child_count] = *body;
            node->child_count++;
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
            root->children = (ASTNode*)tracked_realloc(root->children, (root->child_count + 1) * sizeof(ASTNode), __FILE__, __LINE__, "parser_parse_root");
            if (!root->children) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                parser_free_ast(root);
                parser_free_ast(node);
                return NULL;
            }
            root->children[root->child_count] = *node;
            root->child_count++;
        }
    }

    return root;
}

void parser_free_ast(ASTNode* node) {
    if (!node) return;
    
    // Free the next node in the linked list first
    if (node->next) {
        parser_free_ast(node->next);
        node->next = NULL;
    }
    
    // Recursively free all children
    if (node->children && node->child_count > 0) {
        for (int i = 0; i < node->child_count; i++) {
            // Free children recursively - children[i] is an ASTNode struct, not a pointer
            if (node->children[i].children && node->children[i].child_count > 0) {
                for (int j = 0; j < node->children[i].child_count; j++) {
                    if (node->children[i].children[j].text) {
                        tracked_free(node->children[i].children[j].text, __FILE__, __LINE__, "parser_free_ast");
                    }
                }
                tracked_free(node->children[i].children, __FILE__, __LINE__, "parser_free_ast");
            }
            if (node->children[i].text) {
                tracked_free(node->children[i].text, __FILE__, __LINE__, "parser_free_ast");
            }
        }
        tracked_free(node->children, __FILE__, __LINE__, "parser_free_ast");
        node->children = NULL;
    }
    
    // Free text
    if (node->text) {
        tracked_free(node->text, __FILE__, __LINE__, "parser_free_ast");
        node->text = NULL;
    }
    
    tracked_free(node, __FILE__, __LINE__, "parser_free_ast");
} 