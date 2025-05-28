#include "ast.h"
#include "myco.h"
#include <stdlib.h>
#include <string.h>

static ASTNode *ast_node_new(ASTNodeType type, int line, int column) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

ASTNode *ast_literal_number(double value, int line, int column) {
    ASTNode *node = ast_node_new(AST_LITERAL_NUMBER, line, column);
    node->as.number = value;
    return node;
}

ASTNode *ast_literal_string(const char *value, int line, int column) {
    ASTNode *node = ast_node_new(AST_LITERAL_STRING, line, column);
    node->as.string = strdup(value);
    return node;
}

ASTNode *ast_literal_bool(bool value, int line, int column) {
    ASTNode *node = ast_node_new(AST_LITERAL_BOOL, line, column);
    node->as.boolean = value;
    return node;
}

ASTNode *ast_identifier(const char *name, int line, int column) {
    ASTNode *node = ast_node_new(AST_IDENTIFIER, line, column);
    node->as.identifier = strdup(name);
    return node;
}

ASTNode *ast_binary(ASTNode *left, TokenType op, ASTNode *right, int line, int column) {
    ASTNode *node = ast_node_new(AST_BINARY, line, column);
    node->as.binary.left = left;
    node->as.binary.op = op;
    node->as.binary.right = right;
    return node;
}

ASTNode *ast_unary(TokenType op, ASTNode *expr, int line, int column) {
    ASTNode *node = ast_node_new(AST_UNARY, line, column);
    node->as.unary.op = op;
    node->as.unary.expr = expr;
    return node;
}

ASTNode *ast_call(ASTNode *callee, ASTNode **args, size_t arg_count, int line, int column) {
    ASTNode *node = ast_node_new(AST_CALL, line, column);
    node->as.call.callee = callee;
    node->as.call.args = args;
    node->as.call.arg_count = arg_count;
    return node;
}

ASTNode *ast_member(ASTNode *object, const char *property, int line, int column) {
    ASTNode *node = ast_node_new(AST_MEMBER, line, column);
    node->as.member.object = object;
    node->as.member.property = strdup(property);
    return node;
}

ASTNode *ast_index(ASTNode *array, ASTNode *index, int line, int column) {
    ASTNode *node = ast_node_new(AST_INDEX, line, column);
    node->as.index.array = array;
    node->as.index.index = index;
    return node;
}

ASTNode *ast_var_decl_with_type(const char *name, bool is_const, TokenType type, ASTNode *initializer, int line, int column) {
    ASTNode *node = ast_node_new(AST_VAR_DECL, line, column);
    node->as.var_decl.name = strdup(name);
    node->as.var_decl.is_const = is_const;
    node->as.var_decl.type = type;
    node->as.var_decl.initializer = initializer;
    return node;
}

ASTNode *ast_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch, int line, int column) {
    ASTNode *node = ast_node_new(AST_IF, line, column);
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_branch = then_branch;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

ASTNode *ast_while(ASTNode *condition, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_new(AST_WHILE, line, column);
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

ASTNode *ast_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_new(AST_FOR, line, column);
    node->as.for_stmt.init = init;
    node->as.for_stmt.condition = condition;
    node->as.for_stmt.update = update;
    node->as.for_stmt.body = body;
    return node;
}

ASTNode *ast_for_in(const char *var, ASTNode *iterable, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_new(AST_FOR_IN, line, column);
    node->as.for_in.var = strdup(var);
    node->as.for_in.iterable = iterable;
    node->as.for_in.body = body;
    return node;
}

ASTNode *ast_function_with_types(const char *name, char **params, TokenType *param_types, size_t param_count, TokenType return_type, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_new(AST_FUNCTION, line, column);
    node->as.function.name = strdup(name);
    node->as.function.params = params;
    node->as.function.param_types = param_types;
    node->as.function.param_count = param_count;
    node->as.function.return_type = return_type;
    node->as.function.body = body;
    return node;
}

ASTNode *ast_return(ASTNode *value, int line, int column) {
    ASTNode *node = ast_node_new(AST_RETURN, line, column);
    node->as.return_stmt.value = value;
    return node;
}

ASTNode *ast_try(ASTNode *try_block, const char *catch_var, ASTNode *catch_block, int line, int column) {
    ASTNode *node = ast_node_new(AST_TRY, line, column);
    node->as.try_stmt.try_block = try_block;
    node->as.try_stmt.catch_var = strdup(catch_var);
    node->as.try_stmt.catch_block = catch_block;
    return node;
}

ASTNode *ast_switch(ASTNode *value, ASTNode **cases, size_t case_count, int line, int column) {
    ASTNode *node = ast_node_new(AST_SWITCH, line, column);
    node->as.switch_stmt.value = value;
    node->as.switch_stmt.cases = cases;
    node->as.switch_stmt.case_count = case_count;
    return node;
}

ASTNode *ast_case(ASTNode *condition, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_new(AST_CASE, line, column);
    node->as.case_stmt.condition = condition;
    node->as.case_stmt.body = body;
    return node;
}

ASTNode *ast_multi_assign(char **names, size_t name_count, ASTNode **values, size_t value_count, int line, int column) {
    ASTNode *node = ast_node_new(AST_MULTI_ASSIGN, line, column);
    node->as.multi_assign.names = names;
    node->as.multi_assign.name_count = name_count;
    node->as.multi_assign.values = values;
    node->as.multi_assign.value_count = value_count;
    return node;
}

ASTNode *ast_program(ASTNode **statements, size_t statement_count) {
    ASTNode *node = ast_node_new(AST_PROGRAM, 0, 0);
    node->as.program.statements = statements;
    node->as.program.statement_count = statement_count;
    return node;
}

void ast_free(ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case AST_LITERAL_STRING:
            free(node->as.string);
            break;
            
        case AST_IDENTIFIER:
            free(node->as.identifier);
            break;
            
        case AST_BINARY:
            ast_free(node->as.binary.left);
            ast_free(node->as.binary.right);
            break;
            
        case AST_UNARY:
            ast_free(node->as.unary.expr);
            break;
            
        case AST_CALL:
            ast_free(node->as.call.callee);
            for (size_t i = 0; i < node->as.call.arg_count; i++) {
                ast_free(node->as.call.args[i]);
            }
            free(node->as.call.args);
            break;
            
        case AST_MEMBER:
            ast_free(node->as.member.object);
            free(node->as.member.property);
            break;
            
        case AST_INDEX:
            ast_free(node->as.index.array);
            ast_free(node->as.index.index);
            break;
            
        case AST_VAR_DECL:
            free(node->as.var_decl.name);
            ast_free(node->as.var_decl.initializer);
            break;
            
        case AST_IF:
            ast_free(node->as.if_stmt.condition);
            ast_free(node->as.if_stmt.then_branch);
            ast_free(node->as.if_stmt.else_branch);
            break;
            
        case AST_WHILE:
            ast_free(node->as.while_stmt.condition);
            ast_free(node->as.while_stmt.body);
            break;
            
        case AST_FOR:
            ast_free(node->as.for_stmt.init);
            ast_free(node->as.for_stmt.condition);
            ast_free(node->as.for_stmt.update);
            ast_free(node->as.for_stmt.body);
            break;
            
        case AST_FOR_IN:
            free(node->as.for_in.var);
            ast_free(node->as.for_in.iterable);
            ast_free(node->as.for_in.body);
            break;
            
        case AST_FUNCTION:
            free(node->as.function.name);
            for (size_t i = 0; i < node->as.function.param_count; i++) {
                free(node->as.function.params[i]);
            }
            free(node->as.function.params);
            free(node->as.function.param_types);
            ast_free(node->as.function.body);
            break;
            
        case AST_RETURN:
            ast_free(node->as.return_stmt.value);
            break;
            
        case AST_TRY:
            ast_free(node->as.try_stmt.try_block);
            free(node->as.try_stmt.catch_var);
            ast_free(node->as.try_stmt.catch_block);
            break;
            
        case AST_SWITCH:
            ast_free(node->as.switch_stmt.value);
            for (size_t i = 0; i < node->as.switch_stmt.case_count; i++) {
                ast_free(node->as.switch_stmt.cases[i]);
            }
            free(node->as.switch_stmt.cases);
            break;
            
        case AST_CASE:
            ast_free(node->as.case_stmt.condition);
            ast_free(node->as.case_stmt.body);
            break;
            
        case AST_MULTI_ASSIGN:
            for (size_t i = 0; i < node->as.multi_assign.name_count; i++) {
                free(node->as.multi_assign.names[i]);
            }
            free(node->as.multi_assign.names);
            for (size_t i = 0; i < node->as.multi_assign.value_count; i++) {
                ast_free(node->as.multi_assign.values[i]);
            }
            free(node->as.multi_assign.values);
            break;
            
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.program.statement_count; i++) {
                ast_free(node->as.program.statements[i]);
            }
            free(node->as.program.statements);
            break;
            
        default:
            break;
    }
    
    free(node);
} 