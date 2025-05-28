#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myco.h"

// Forward declarations
static Node* parse_block();
static Node* parse_statement();
static Node* parse_expression();
static Node* parse_primary();
static Node* parse_declaration();
static Node* parse_var_decl();
static Node* parse_const_decl();
static Node* parse_func_decl();
static Node* parse_if();
static Node* parse_while();
static Node* parse_for();
static Node* parse_switch();
static Node* parse_try();
static Node* parse_print();
static Node* parse_assignment(Node* left);
static Node* parse_binary(Node* left, int min_prec);
static Node* parse_call(Node* callee);
static Node* parse_return();
static Node* make_binary(Node* left, Token op, Node* right);
static Node* make_literal(double number, const char* string);

// Parser state
static Lexer lexer;
static Token current;
static Token previous;

static void advance() {
    previous = current;
    current = lexer_next_token(&lexer);
}

static bool check(TokenType type) {
    return current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void expect(TokenType type, const char* msg) {
    if (!match(type)) {
        fprintf(stderr, "[Line %d] Error: %s\n", current.line, msg);
        exit(1);
    }
}

static Node* make_node(NodeType type) {
    Node* node = calloc(1, sizeof(Node));
    node->type = type;
    return node;
}

// Precedence table for binary operators
static int get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_DOT_DOT: return 7; // string concat
        case TOKEN_STAR: case TOKEN_SLASH: case TOKEN_PERCENT: return 6;
        case TOKEN_PLUS: case TOKEN_MINUS: return 5;
        case TOKEN_LESS: case TOKEN_LESS_EQUAL: case TOKEN_GREATER: case TOKEN_GREATER_EQUAL: return 4;
        case TOKEN_EQUAL: case TOKEN_BANG_EQUAL: return 3;
        case TOKEN_ASSIGN: return 2;
        default: return 0;
    }
}

static Node* parse_primary() {
    if (match(TOKEN_NUMBER)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.*s", previous.length, previous.start);
        double num = atof(buf);
        Node* node = make_literal(num, NULL);
        return node;
    }
    if (match(TOKEN_STRING)) {
        char* str = malloc(previous.length + 1);
        strncpy(str, previous.start, previous.length);
        str[previous.length] = '\0';
        Node* node = make_literal(0, str);
        return node;
    }
    if (match(TOKEN_IDENTIFIER)) {
        Node* node = make_node(NODE_VARIABLE);
        node->value.identifier = previous;
        if (check(TOKEN_LEFT_PAREN)) {
            return parse_call(node);
        }
        return node;
    }
    if (match(TOKEN_LEFT_PAREN)) {
        Node* expr = parse_expression();
        expect(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
        return expr;
    }
    if (match(TOKEN_LEFT_BRACKET)) {
        // List literal
        Node* list = make_node(NODE_LIST);
        Node* head = NULL;
        Node* tail = NULL;
        if (!check(TOKEN_RIGHT_BRACKET)) {
            do {
                Node* elem = parse_expression();
                if (!head) head = tail = elem;
                else { tail->right = elem; tail = elem; }
            } while (match(TOKEN_COMMA));
        }
        expect(TOKEN_RIGHT_BRACKET, "Expect ']' after list");
        list->left = head;
        return list;
    }
    if (match(TOKEN_LEFT_BRACE)) {
        // Map literal
        Node* map = make_node(NODE_MAP);
        Node* head = NULL;
        Node* tail = NULL;
        if (!check(TOKEN_RIGHT_BRACE)) {
            do {
                Node* key = parse_expression();
                expect(TOKEN_COLON, "Expect ':' after map key");
                Node* value = parse_expression();
                Node* pair = make_node(NODE_ASSIGNMENT);
                pair->left = key;
                pair->right = value;
                if (!head) head = tail = pair;
                else { tail->right = pair; tail = pair; }
            } while (match(TOKEN_COMMA));
        }
        expect(TOKEN_RIGHT_BRACE, "Expect '}' after map");
        map->left = head;
        return map;
    }
    fprintf(stderr, "[Line %d] Error: Unexpected token in expression\n", current.line);
    exit(1);
}

static Node* parse_call(Node* callee) {
    Node* node = make_node(NODE_CALL);
    node->value.call.callee = callee;
    advance(); // consume '('
    Node* arg_head = NULL;
    Node* arg_tail = NULL;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            Node* arg = parse_expression();
            if (!arg_head) {
                arg_head = arg_tail = arg;
            } else {
                arg_tail->right = arg;
                arg_tail = arg;
            }
        } while (match(TOKEN_COMMA));
    }
    expect(TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
    node->value.call.args = arg_head;
    return node;
}

static Node* parse_unary() {
    if (match(TOKEN_MINUS) || match(TOKEN_BANG)) {
        Node* node = make_node(NODE_UNARY);
        node->left = parse_unary();
        node->value.identifier = previous;
        return node;
    }
    return parse_primary();
}

static Node* parse_binary(Node* left, int min_prec) {
    while (1) {
        int prec = get_precedence(current.type);
        if (prec < min_prec) break;
        Token op_token = current; // Save the operator token
        advance();
        Node* right = parse_unary();
        int next_prec = get_precedence(current.type);
        while (prec < next_prec) {
            right = parse_binary(right, prec + 1);
            next_prec = get_precedence(current.type);
        }
        Node* bin = make_binary(left, op_token, right); // Use op_token, not current
        left = bin;
    }
    return left;
}

static Node* make_binary(Node* left, Token op, Node* right) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_BINARY;
    node->left = left;
    node->right = right;
    node->value.identifier = op;
    return node;
}

static Node* parse_assignment(Node* left) {
    if (match(TOKEN_ASSIGN)) {
        Node* node = make_node(NODE_ASSIGNMENT);
        node->left = left;
        node->right = parse_expression();
        return node;
    }
    return left;
}

static Node* parse_expression() {
    Node* left = parse_unary();
    left = parse_binary(left, 1);
    left = parse_assignment(left);
    return left;
}

static Node* parse_print() {
    advance(); // consume 'print'
    Node* node = make_node(NODE_PRINT);
    node->left = parse_expression();
    // Make semicolon optional after print statement
    match(TOKEN_SEMICOLON);
    return node;
}

static Node* parse_var_decl() {
    advance(); // consume 'let' or 'var'
    expect(TOKEN_IDENTIFIER, "Expect variable name");
    Node* var = make_node(NODE_VARIABLE);
    var->value.identifier = previous;
    Node* init = NULL;
    if (match(TOKEN_ASSIGN)) {
        init = parse_expression();
    }
    match(TOKEN_SEMICOLON); // Semicolon is now optional
    Node* node = make_node(NODE_VAR_DECL);
    node->left = var;
    node->right = init;
    return node;
}

static Node* parse_const_decl() {
    advance(); // consume 'const'
    expect(TOKEN_IDENTIFIER, "Expect constant name");
    Node* var = make_node(NODE_VARIABLE);
    var->value.identifier = previous;
    expect(TOKEN_ASSIGN, "Expect '=' after constant name");
    Node* init = parse_expression();
    match(TOKEN_SEMICOLON); // Semicolon is now optional
    Node* node = make_node(NODE_CONST_DECL);
    node->left = var;
    node->right = init;
    return node;
}

static Node* parse_return() {
    advance(); // consume 'return'
    Node* expr = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        expr = parse_expression();
    }
    match(TOKEN_SEMICOLON); // Semicolon is now optional
    Node* node = make_node(NODE_RETURN);
    node->left = expr;
    return node;
}

// Helper to skip semicolons and newlines after a block header (colon)
static void skip_block_separators() {
    // Skip any semicolons or colons that might appear after a block header
    while (check(TOKEN_SEMICOLON) || check(TOKEN_COLON)) {
        advance();
    }
}

static Node* parse_if() {
    advance(); // consume 'if'
    Node* cond = parse_expression();
    expect(TOKEN_COLON, "Expect ':' after if condition");
    skip_block_separators();
    
    // Parse the then block
    Node* then_block = parse_block();
    
    // Parse the else block if it exists
    Node* else_block = NULL;
    if (check(TOKEN_ELSEIF)) {
        else_block = parse_if(); // recursively build the chain
    } else if (check(TOKEN_ELSE)) {
        advance();
        expect(TOKEN_COLON, "Expect ':' after else");
        skip_block_separators();
        else_block = parse_block();
    }
    
    expect(TOKEN_END, "Expect 'end' after if block");
    advance();
    
    // Create the if node
    Node* node = make_node(NODE_IF);
    node->left = cond;
    node->right = then_block;
    
    // Link the else block if it exists
    if (else_block) {
        Node* else_node = make_node(NODE_BLOCK);
        else_node->left = else_block;
        node->right->right = else_node;
    }
    
    return node;
}

static Node* parse_while() {
    advance(); // consume 'while'
    Node* cond = parse_expression();
    expect(TOKEN_COLON, "Expect ':' after while condition");
    skip_block_separators();
    Node* body = parse_block();
    expect(TOKEN_END, "Expect 'end' after while block");
    advance();
    Node* node = make_node(NODE_WHILE);
    node->left = cond;
    node->right = body;
    return node;
}

static Node* parse_for() {
    advance(); // consume 'for'
    expect(TOKEN_IDENTIFIER, "Expect variable name after 'for'");
    Node* var = make_node(NODE_VARIABLE);
    var->value.identifier = previous;
    expect(TOKEN_IN, "Expect 'in' after for variable");
    Node* iterable = parse_expression();
    expect(TOKEN_COLON, "Expect ':' after for-in");
    skip_block_separators();
    Node* body = parse_block();
    expect(TOKEN_END, "Expect 'end' after for block");
    advance();
    Node* node = make_node(NODE_FOR);
    node->left = var;
    node->right = make_node(NODE_BLOCK);
    node->right->left = iterable;
    node->right->right = body;
    return node;
}

static Node* parse_func_decl() {
    advance(); // consume 'func'
    expect(TOKEN_IDENTIFIER, "Expect function name");
    Node* func = make_node(NODE_FUNCTION);
    func->value.identifier = previous;
    expect(TOKEN_LEFT_PAREN, "Expect '(' after function name");
    // Parse parameters
    Node* param_head = NULL;
    Node* param_tail = NULL;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expect(TOKEN_IDENTIFIER, "Expect parameter name");
            Node* param = make_node(NODE_VARIABLE);
            param->value.identifier = previous;
            if (!param_head) param_head = param_tail = param;
            else { param_tail->right = param; param_tail = param; }
        } while (match(TOKEN_COMMA));
    }
    expect(TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
    expect(TOKEN_COLON, "Expect ':' after function signature");
    skip_block_separators();
    Node* body = parse_block();
    expect(TOKEN_END, "Expect 'end' after function block");
    advance();
    func->value.function.params = param_head;
    func->value.function.body = body;
    return func;
}

static Node* parse_switch() {
    advance(); // consume 'switch'
    Node* expr = parse_expression();
    expect(TOKEN_COLON, "Expect ':' after switch expression");
    skip_block_separators();
    Node* head = NULL;
    Node* tail = NULL;
    while (check(TOKEN_CASE)) {
        advance();
        Node* case_val = parse_expression();
        expect(TOKEN_COLON, "Expect ':' after case value");
        skip_block_separators();
        Node* case_block = parse_block();
        Node* case_node = make_node(NODE_CASE);
        case_node->left = case_val;
        case_node->right = case_block;
        if (!head) head = tail = case_node;
        else { tail->right = case_node; tail = case_node; }
    }
    Node* default_block = NULL;
    if (check(TOKEN_DEFAULT)) {
        advance();
        expect(TOKEN_COLON, "Expect ':' after default");
        skip_block_separators();
        default_block = parse_block();
    }
    expect(TOKEN_END, "Expect 'end' after switch block");
    advance();
    Node* node = make_node(NODE_SWITCH);
    node->left = expr;
    node->right = head;
    if (default_block) {
        Node* def = make_node(NODE_CASE);
        def->left = NULL;
        def->right = default_block;
        if (!head) node->right = def;
        else tail->right = def;
    }
    return node;
}

static Node* parse_try() {
    advance(); // consume 'try'
    skip_block_separators();
    Node* try_block = parse_block();
    Node* catch_block = NULL;
    if (check(TOKEN_CATCH)) {
        advance();
        skip_block_separators();
        catch_block = parse_block();
    }
    expect(TOKEN_END, "Expect 'end' after try/catch block");
    advance();
    Node* node = make_node(NODE_TRY);
    node->left = try_block;
    node->right = catch_block;
    return node;
}

static bool is_expression_starter(TokenType type) {
    // Exclude statement-only keywords like print, return, etc.
    switch (type) {
        case TOKEN_PRINT:
        case TOKEN_RETURN:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_FOR:
        case TOKEN_FUNC:
        case TOKEN_SWITCH:
        case TOKEN_TRY:
        case TOKEN_LET:
        case TOKEN_VAR:
        case TOKEN_CONST:
            return false;
        default:
            return type == TOKEN_NUMBER || type == TOKEN_STRING || type == TOKEN_IDENTIFIER ||
                   type == TOKEN_LEFT_PAREN || type == TOKEN_LEFT_BRACKET || type == TOKEN_LEFT_BRACE ||
                   type == TOKEN_MINUS || type == TOKEN_BANG;
    }
}

static Node* parse_statement() {
    if (check(TOKEN_PRINT)) return parse_print();
    if (check(TOKEN_LET) || check(TOKEN_VAR)) return parse_var_decl();
    if (check(TOKEN_CONST)) return parse_const_decl();
    if (check(TOKEN_RETURN)) return parse_return();
    if (check(TOKEN_IF)) return parse_if();
    if (check(TOKEN_WHILE)) return parse_while();
    if (check(TOKEN_FOR)) return parse_for();
    if (check(TOKEN_FUNC)) return parse_func_decl();
    if (check(TOKEN_SWITCH)) return parse_switch();
    if (check(TOKEN_TRY)) return parse_try();
    // Only parse an expression statement if the current token is a valid expression starter
    if (is_expression_starter(current.type)) {
        Node* expr = parse_expression();
        // Make semicolon optional after expression statement
        match(TOKEN_SEMICOLON);
        return expr;
    }
    fprintf(stderr, "[Line %d] Error: Unexpected token in statement\n", current.line);
    exit(1);
}

static Node* parse_block() {
    Node* block = make_node(NODE_BLOCK);
    Node* head = NULL;
    Node* tail = NULL;
    
    while (!check(TOKEN_EOF) && !check(TOKEN_END) && !check(TOKEN_ELSE) && !check(TOKEN_ELSEIF) && !check(TOKEN_CATCH) && !check(TOKEN_CASE) && !check(TOKEN_DEFAULT)) {
        // Skip any semicolons or colons between statements
        while (check(TOKEN_SEMICOLON) || check(TOKEN_COLON)) {
            advance();
        }
        
        // Check if we've reached the end of the block
        if (check(TOKEN_EOF) || check(TOKEN_END) || check(TOKEN_ELSE) || check(TOKEN_ELSEIF) || check(TOKEN_CATCH) || check(TOKEN_CASE) || check(TOKEN_DEFAULT)) {
            break;
        }
        
        // Parse the statement
        Node* stmt = parse_statement();
        
        // Link the statement directly to the block
        if (!head) {
            head = tail = stmt;
        } else {
            tail->right = stmt;
            tail = stmt;
        }
    }
    
    block->left = head;
    return block;
}

static Node* make_literal(double number, const char* string) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_LITERAL;
    node->left = NULL;
    node->right = NULL;
    node->number = number;
    node->string = string;
    return node;
}

Node* parse(const char* source) {
    lexer_init(&lexer, source);
    advance();
    return parse_block();
} 