#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 256
#define MAX_FUNCS 64

typedef struct {
    char* name;
    int64_t value;
} Var;

typedef struct {
    char* name;
    ASTNode* params;
    ASTNode* body;
} Func;

static Var vars[MAX_VARS];
static int var_count = 0;

static Func funcs[MAX_FUNCS];
static int func_count = 0;

static int64_t lookup_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return vars[i].value;
        }
    }
    fprintf(stderr, "Undefined variable: %s\n", name);
    return 0;
}

static void set_var(const char* name, int64_t value) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            vars[i].value = value;
            return;
        }
    }
    if (var_count < MAX_VARS) {
        vars[var_count].name = strdup(name);
        vars[var_count].value = value;
        var_count++;
    }
}

static Func* lookup_func(const char* name) {
    for (int i = 0; i < func_count; i++) {
        if (strcmp(funcs[i].name, name) == 0) {
            return &funcs[i];
        }
    }
    return NULL;
}

static void store_func(const char* name, ASTNode* params, ASTNode* body) {
    if (func_count < MAX_FUNCS) {
        funcs[func_count].name = strdup(name);
        funcs[func_count].params = params;
        funcs[func_count].body = body;
        func_count++;
    }
}

// Function to interpret an AST node
static int64_t interpret_node(ASTNode* node) {
    if (!node) return 0;

    switch (node->type) {
        case NODE_PROGRAM:
            if (node->as.block.statements) {
                int64_t result = 0;
                for (ASTNode* stmt = node->as.block.statements; stmt; stmt = stmt->next) {
                    result = interpret_node(stmt);
                }
                return result;
            }
            return 0;
        case NODE_LITERAL:
            switch (node->as.literal.value_type) {
                case TOKEN_INTEGER:
                    return node->as.literal.int_value;
                case TOKEN_FLOAT_LITERAL:
                    // For simplicity, convert float to int
                    return (int64_t)node->as.literal.float_value;
                case TOKEN_STRING_LITERAL:
                    printf("%s", node->as.literal.string_value);
                    return 0;
                case TOKEN_TRUE:
                    return 1;
                case TOKEN_FALSE:
                    return 0;
                default:
                    fprintf(stderr, "Error: Unknown literal type\n");
                    return 0;
            }
            break;

        case NODE_IDENTIFIER:
            return lookup_var(node->as.literal.string_value);

        case NODE_BINARY_OP:
            {
                int64_t left = interpret_node(node->as.binary.left);
                int64_t right = interpret_node(node->as.binary.right);
                switch (node->as.binary.operator) {
                    case TOKEN_PLUS: return left + right;
                    case TOKEN_MINUS: return left - right;
                    case TOKEN_MULTIPLY: return left * right;
                    case TOKEN_DIVIDE: return right != 0 ? left / right : 0;
                    case TOKEN_MODULO: return left % right;
                    case TOKEN_EQUALS: return left == right;
                    case TOKEN_NOT_EQUALS: return left != right;
                    case TOKEN_LESS: return left < right;
                    case TOKEN_GREATER: return left > right;
                    case TOKEN_LESS_EQUALS: return left <= right;
                    case TOKEN_GREATER_EQUALS: return left >= right;
                    case TOKEN_AND: return left && right;
                    case TOKEN_OR: return left || right;
                    default:
                        fprintf(stderr, "Error: Unknown binary operator\n");
                        return 0;
                }
            }
            break;

        case NODE_UNARY_OP:
            {
                int64_t operand = interpret_node(node->as.unary.operand);
                switch (node->as.unary.operator) {
                    case TOKEN_MINUS: return -operand;
                    case TOKEN_NOT: return !operand;
                    default:
                        fprintf(stderr, "Error: Unknown unary operator\n");
                        return 0;
                }
            }
            break;

        case NODE_VAR_DECL:
            {
                int64_t val = interpret_node(node->as.var_decl.initializer);
                set_var(node->as.var_decl.name, val);
                return val;
            }

        case NODE_FUNCTION_DEF:
            store_func(node->as.function.name, node->as.function.params, node->as.function.body);
            return 0;

        case NODE_IF_STMT:
            {
                int64_t condition = interpret_node(node->as.if_stmt.condition);
                if (condition) {
                    return interpret_node(node->as.if_stmt.then_branch);
                } else {
                    return interpret_node(node->as.if_stmt.else_branch);
                }
            }
            break;

        case NODE_WHILE_STMT:
            {
                int64_t result = 0;
                while (interpret_node(node->as.while_stmt.condition)) {
                    result = interpret_node(node->as.while_stmt.body);
                }
                return result;
            }
            break;

        case NODE_FOR_STMT:
            {
                int64_t result = 0;
                // For simplicity, assume a range-based for loop
                int64_t start = interpret_node(node->as.for_stmt.start);
                int64_t end = interpret_node(node->as.for_stmt.end);
                for (int64_t i = start; i < end; i++) {
                    result = interpret_node(node->as.for_stmt.body);
                }
                return result;
            }
            break;

        case NODE_SWITCH_STMT:
            {
                int64_t value = interpret_node(node->as.switch_stmt.value);
                // For simplicity, assume a single case
                if (value == interpret_node(node->as.switch_stmt.case_value)) {
                    return interpret_node(node->as.switch_stmt.case_body);
                } else {
                    return interpret_node(node->as.switch_stmt.default_body);
                }
            }
            break;

        case NODE_TRY_STMT:
            {
                int64_t result = interpret_node(node->as.try_stmt.try_body);
                if (result == 0) {
                    return interpret_node(node->as.try_stmt.catch_body);
                }
                return result;
            }
            break;

        case NODE_BLOCK:
            {
                int64_t result = 0;
                for (ASTNode* stmt = node->as.block.statements; stmt; stmt = stmt->next) {
                    result = interpret_node(stmt);
                }
                return result;
            }
            break;

        case NODE_FUNCTION_CALL:
            {
                Func* func = lookup_func(node->as.function.name);
                if (!func) {
                    if (strcmp(node->as.function.name, "print") == 0) {
                        int64_t val = interpret_node(node->as.function.params);
                        printf("%lld\n", val);
                        return val;
                    }
                    fprintf(stderr, "Unknown function: %s\n", node->as.function.name);
                    return 0;
                }

                // Save current variable state
                Var old_vars[MAX_VARS];
                int old_var_count = var_count;
                memcpy(old_vars, vars, sizeof(vars));

                // Set up parameters
                ASTNode* param = func->params;
                ASTNode* arg = node->as.function.params;
                while (param && arg) {
                    set_var(param->as.var_decl.name, interpret_node(arg));
                    param = param->next;
                    arg = arg->next;
                }

                // Execute function body
                int64_t result = interpret_node(func->body);

                // Restore variable state
                for (int i = 0; i < var_count; i++) {
                    free(vars[i].name);
                }
                var_count = old_var_count;
                memcpy(vars, old_vars, sizeof(vars));

                return result;
            }

        default:
            fprintf(stderr, "Error: Unknown node type\n");
            return 0;
    }
}

// Function to interpret the entire AST
int64_t interpret_ast(ASTNode* ast) {
    return interpret_node(ast);
} 