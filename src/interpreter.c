#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 256

typedef enum { VAR_INT, VAR_STRING } VarType;
typedef struct {
    char* name;
    VarType type;
    int64_t int_value;
    char* str_value;
} Var;

static Var vars[MAX_VARS];
static int var_count = 0;

static Var* find_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return &vars[i];
        }
    }
    return NULL;
}

static int64_t lookup_var_int(const char* name) {
    Var* var = find_var(name);
    if (var && var->type == VAR_INT) {
        return var->int_value;
    }
    fprintf(stderr, "Undefined or non-int variable: %s\n", name);
    return 0;
}

static const char* lookup_var_str(const char* name) {
    Var* var = find_var(name);
    if (var && var->type == VAR_STRING) {
        return var->str_value;
    }
    fprintf(stderr, "Undefined or non-string variable: %s\n", name);
    return "";
}

static void set_var_int(const char* name, int64_t value) {
    Var* var = find_var(name);
    if (var) {
        if (var->type == VAR_STRING && var->str_value) free(var->str_value);
        var->type = VAR_INT;
        var->int_value = value;
        var->str_value = NULL;
        return;
    }
    if (var_count < MAX_VARS) {
        vars[var_count].name = strdup(name);
        vars[var_count].type = VAR_INT;
        vars[var_count].int_value = value;
        vars[var_count].str_value = NULL;
        var_count++;
    }
}

static void set_var_str(const char* name, const char* value) {
    Var* var = find_var(name);
    if (var) {
        if (var->type == VAR_STRING && var->str_value) free(var->str_value);
        var->type = VAR_STRING;
        var->str_value = strdup(value);
        var->int_value = 0;
        return;
    }
    if (var_count < MAX_VARS) {
        vars[var_count].name = strdup(name);
        vars[var_count].type = VAR_STRING;
        vars[var_count].str_value = strdup(value);
        vars[var_count].int_value = 0;
        var_count++;
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
            {
                Var* var = find_var(node->as.literal.string_value);
                if (!var) {
                    fprintf(stderr, "Undefined variable: %s\n", node->as.literal.string_value);
                    return 0;
                }
                if (var->type == VAR_STRING) {
                    printf("%s", var->str_value);
                    return 0;
                } else {
                    return var->int_value;
                }
            }

        case NODE_BINARY_OP:
            {
                if (node->as.binary.operator == TOKEN_DOT_DOT) {
                    char* left_str = NULL;
                    char* right_str = NULL;
                    // Left operand
                    if (node->as.binary.left->type == NODE_LITERAL && node->as.binary.left->as.literal.value_type == TOKEN_STRING_LITERAL) {
                        left_str = strdup(node->as.binary.left->as.literal.string_value);
                    } else if (node->as.binary.left->type == NODE_IDENTIFIER) {
                        Var* var = find_var(node->as.binary.left->as.literal.string_value);
                        if (var && var->type == VAR_STRING) left_str = strdup(var->str_value);
                        else if (var && var->type == VAR_INT) {
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%lld", var->int_value);
                            left_str = strdup(buf);
                        }
                    }
                    // Right operand
                    if (node->as.binary.right->type == NODE_LITERAL && node->as.binary.right->as.literal.value_type == TOKEN_STRING_LITERAL) {
                        right_str = strdup(node->as.binary.right->as.literal.string_value);
                    } else if (node->as.binary.right->type == NODE_IDENTIFIER) {
                        Var* var = find_var(node->as.binary.right->as.literal.string_value);
                        if (var && var->type == VAR_STRING) right_str = strdup(var->str_value);
                        else if (var && var->type == VAR_INT) {
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%lld", var->int_value);
                            right_str = strdup(buf);
                        }
                    }
                    if (left_str && right_str) {
                        size_t len1 = strlen(left_str);
                        size_t len2 = strlen(right_str);
                        char* result = malloc(len1 + len2 + 1);
                        strcpy(result, left_str);
                        strcat(result, right_str);
                        printf("%s", result);
                        free(result);
                        free(left_str);
                        free(right_str);
                        return 0;
                    } else {
                        fprintf(stderr, "Error: Concatenation requires string literals or string variables.\n");
                        if (left_str) free(left_str);
                        if (right_str) free(right_str);
                        return 0;
                    }
                }
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
                ASTNode* init = node->as.var_decl.initializer;
                if (init->type == NODE_LITERAL && init->as.literal.value_type == TOKEN_STRING_LITERAL) {
                    set_var_str(node->as.var_decl.name, init->as.literal.string_value);
                    return 0;
                } else if (init->type == NODE_BINARY_OP && init->as.binary.operator == TOKEN_DOT_DOT) {
                    char* left_str = NULL;
                    char* right_str = NULL;
                    // Left operand
                    if (init->as.binary.left->type == NODE_LITERAL && init->as.binary.left->as.literal.value_type == TOKEN_STRING_LITERAL) {
                        left_str = strdup(init->as.binary.left->as.literal.string_value);
                    } else if (init->as.binary.left->type == NODE_IDENTIFIER) {
                        Var* var = find_var(init->as.binary.left->as.literal.string_value);
                        if (var && var->type == VAR_STRING) left_str = strdup(var->str_value);
                        else if (var && var->type == VAR_INT) {
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%lld", var->int_value);
                            left_str = strdup(buf);
                        }
                    }
                    // Right operand
                    if (init->as.binary.right->type == NODE_LITERAL && init->as.binary.right->as.literal.value_type == TOKEN_STRING_LITERAL) {
                        right_str = strdup(init->as.binary.right->as.literal.string_value);
                    } else if (init->as.binary.right->type == NODE_IDENTIFIER) {
                        Var* var = find_var(init->as.binary.right->as.literal.string_value);
                        if (var && var->type == VAR_STRING) right_str = strdup(var->str_value);
                        else if (var && var->type == VAR_INT) {
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%lld", var->int_value);
                            right_str = strdup(buf);
                        }
                    }
                    if (left_str && right_str) {
                        size_t len1 = strlen(left_str);
                        size_t len2 = strlen(right_str);
                        char* result = malloc(len1 + len2 + 1);
                        strcpy(result, left_str);
                        strcat(result, right_str);
                        set_var_str(node->as.var_decl.name, result);
                        free(result);
                        free(left_str);
                        free(right_str);
                        return 0;
                    } else {
                        fprintf(stderr, "Error: Concatenation requires string literals or string variables.\n");
                        if (left_str) free(left_str);
                        if (right_str) free(right_str);
                        return 0;
                    }
                } else {
                    int64_t val = interpret_node(init);
                    set_var_int(node->as.var_decl.name, val);
                    return val;
                }
            }

        case NODE_FUNCTION_DEF:
            // For now, just interpret the function body
            return interpret_node(node->as.function.body);

        case NODE_IF_STMT:
            {
                int64_t condition = interpret_node(node->as.if_stmt.condition);
                if (condition) {
                    return interpret_node(node->as.if_stmt.then_branch);
                } else if (node->as.if_stmt.else_branch) {
                    return interpret_node(node->as.if_stmt.else_branch);
                }
                return 0;
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
            if (strcmp(node->as.function.name, "print") == 0) {
                ASTNode* arg = node->as.function.params;
                // String literal
                if (arg->type == NODE_LITERAL && arg->as.literal.value_type == TOKEN_STRING_LITERAL) {
                    interpret_node(arg);
                    printf("\n");
                    return 0;
                }
                // Concatenation
                else if (arg->type == NODE_BINARY_OP && arg->as.binary.operator == TOKEN_DOT_DOT) {
                    interpret_node(arg);
                    printf("\n");
                    return 0;
                }
                // Identifier (variable)
                else if (arg->type == NODE_IDENTIFIER) {
                    Var* var = find_var(arg->as.literal.string_value);
                    if (var && var->type == VAR_STRING) {
                        printf("%s\n", var->str_value);
                        return 0;
                    } else if (var && var->type == VAR_INT) {
                        printf("%lld\n", var->int_value);
                        return 0;
                    } else {
                        fprintf(stderr, "Undefined variable: %s\n", arg->as.literal.string_value);
                        return 0;
                    }
                }
                // Fallback: evaluate and print as int
                else {
                    int64_t val = interpret_node(arg);
                    printf("%lld\n", val);
                    return 0;
                }
            }
            fprintf(stderr, "Unknown function: %s\n", node->as.function.name);
            return 0;

        default:
            fprintf(stderr, "Error: Unknown node type\n");
            return 0;
    }
}

// Function to interpret the entire AST
int64_t interpret_ast(ASTNode* ast) {
    return interpret_node(ast);
} 