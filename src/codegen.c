#include "../include/codegen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CodeGenerator* codegen_init(const char* output_file) {
    CodeGenerator* gen = malloc(sizeof(CodeGenerator));
    if (!gen) return NULL;
    
    gen->output = fopen(output_file, "w");
    if (!gen->output) {
        free(gen);
        return NULL;
    }
    
    gen->indent_level = 0;
    gen->had_error = false;
    
    return gen;
}

void codegen_free(CodeGenerator* gen) {
    if (gen->output) fclose(gen->output);
    free(gen);
}

void codegen_indent(CodeGenerator* gen) {
    gen->indent_level++;
}

void codegen_dedent(CodeGenerator* gen) {
    if (gen->indent_level > 0) gen->indent_level--;
}

void codegen_write(CodeGenerator* gen, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(gen->output, format, args);
    va_end(args);
}

void codegen_write_line(CodeGenerator* gen, const char* format, ...) {
    for (int i = 0; i < gen->indent_level; i++) {
        fprintf(gen->output, "    ");
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(gen->output, format, args);
    va_end(args);
    
    fprintf(gen->output, "\n");
}

void codegen_generate_header(CodeGenerator* gen) {
    codegen_write(gen, "#include <stdio.h>\n");
    codegen_write(gen, "#include <stdlib.h>\n");
    codegen_write(gen, "#include <string.h>\n");
    codegen_write(gen, "#include <stdbool.h>\n");
    codegen_write(gen, "#include <stdint.h>\n\n");
    
    // Runtime functions
    codegen_write(gen, "// Runtime functions\n");
    codegen_write(gen, "void myco_print(const char* str) { printf(\"%%s\", str); }\n");
    codegen_write(gen, "void myco_print_int(int64_t value) { printf(\"%%lld\", value); }\n");
    codegen_write(gen, "void myco_print_float(double value) { printf(\"%%g\", value); }\n");
    codegen_write(gen, "void myco_print_bool(bool value) { printf(value ? \"true\" : \"false\"); }\n");
    codegen_write(gen, "char* myco_str_concat(const char* a, const char* b) {\n");
    codegen_write(gen, "    size_t len_a = strlen(a);\n");
    codegen_write(gen, "    size_t len_b = strlen(b);\n");
    codegen_write(gen, "    char* result = malloc(len_a + len_b + 1);\n");
    codegen_write(gen, "    strcpy(result, a);\n");
    codegen_write(gen, "    strcat(result, b);\n");
    codegen_write(gen, "    return result;\n");
    codegen_write(gen, "}\n\n");
}

void codegen_generate_footer(CodeGenerator* gen) {
    codegen_write(gen, "\nint main() {\n");
    codegen_write(gen, "    // Main program code will be inserted here\n");
    codegen_write(gen, "    return 0;\n");
    codegen_write(gen, "}\n");
}

static void generate_expression(CodeGenerator* gen, ASTNode* node) {
    switch (node->type) {
        case NODE_LITERAL:
            switch (node->as.literal.value_type) {
                case TOKEN_INTEGER:
                    codegen_write(gen, "%lld", node->as.literal.int_value);
                    break;
                case TOKEN_FLOAT_LITERAL:
                    codegen_write(gen, "%g", node->as.literal.float_value);
                    break;
                case TOKEN_STRING_LITERAL:
                    codegen_write(gen, "\"%s\"", node->as.literal.string_value);
                    break;
                case TOKEN_TRUE:
                    codegen_write(gen, "true");
                    break;
                case TOKEN_FALSE:
                    codegen_write(gen, "false");
                    break;
                default:
                    gen->had_error = true;
                    fprintf(stderr, "Error: Unknown literal type\n");
            }
            break;
            
        case NODE_IDENTIFIER:
            codegen_write(gen, "%s", node->as.literal.string_value);
            break;
            
        case NODE_BINARY_OP:
            codegen_write(gen, "(");
            generate_expression(gen, node->as.binary.left);
            
            switch (node->as.binary.operator) {
                case TOKEN_PLUS: codegen_write(gen, " + "); break;
                case TOKEN_MINUS: codegen_write(gen, " - "); break;
                case TOKEN_MULTIPLY: codegen_write(gen, " * "); break;
                case TOKEN_DIVIDE: codegen_write(gen, " / "); break;
                case TOKEN_MODULO: codegen_write(gen, " %% "); break;
                case TOKEN_EQUALS: codegen_write(gen, " == "); break;
                case TOKEN_NOT_EQUALS: codegen_write(gen, " != "); break;
                case TOKEN_LESS: codegen_write(gen, " < "); break;
                case TOKEN_GREATER: codegen_write(gen, " > "); break;
                case TOKEN_LESS_EQUALS: codegen_write(gen, " <= "); break;
                case TOKEN_GREATER_EQUALS: codegen_write(gen, " >= "); break;
                case TOKEN_AND: codegen_write(gen, " && "); break;
                case TOKEN_OR: codegen_write(gen, " || "); break;
                case TOKEN_CONCAT: codegen_write(gen, "myco_str_concat("); break;
                default:
                    gen->had_error = true;
                    fprintf(stderr, "Error: Unknown binary operator\n");
            }
            
            generate_expression(gen, node->as.binary.right);
            
            if (node->as.binary.operator == TOKEN_CONCAT) {
                codegen_write(gen, ")");
            }
            
            codegen_write(gen, ")");
            break;
            
        case NODE_UNARY_OP:
            switch (node->as.unary.operator) {
                case TOKEN_MINUS: codegen_write(gen, "-"); break;
                case TOKEN_NOT: codegen_write(gen, "!"); break;
                default:
                    gen->had_error = true;
                    fprintf(stderr, "Error: Unknown unary operator\n");
            }
            generate_expression(gen, node->as.unary.operand);
            break;
            
        default:
            gen->had_error = true;
            fprintf(stderr, "Error: Unknown expression type\n");
    }
}

static void generate_statement(CodeGenerator* gen, ASTNode* node) {
    switch (node->type) {
        case NODE_VAR_DECL:
            if (node->as.var_decl.type_annotation) {
                // TODO: Handle type annotations
            }
            
            codegen_write_line(gen, "%s %s = ", 
                node->as.var_decl.is_const ? "const" : "",
                node->as.var_decl.name);
            
            if (node->as.var_decl.initializer) {
                generate_expression(gen, node->as.var_decl.initializer);
            } else {
                codegen_write(gen, "0");
            }
            codegen_write(gen, ";\n");
            break;
            
        case NODE_FUNCTION_DEF:
            // TODO: Implement function definition generation
            break;
            
        case NODE_IF_STMT:
            codegen_write_line(gen, "if (");
            generate_expression(gen, node->as.if_stmt.condition);
            codegen_write(gen, ") {\n");
            
            codegen_indent(gen);
            generate_statement(gen, node->as.if_stmt.then_branch);
            codegen_dedent(gen);
            
            if (node->as.if_stmt.else_branch) {
                codegen_write_line(gen, "} else {\n");
                codegen_indent(gen);
                generate_statement(gen, node->as.if_stmt.else_branch);
                codegen_dedent(gen);
            }
            
            codegen_write_line(gen, "}\n");
            break;
            
        case NODE_BLOCK:
            for (ASTNode* stmt = node->as.block.statements; stmt; stmt = stmt->next) {
                generate_statement(gen, stmt);
            }
            break;
            
        default:
            codegen_write_line(gen, "");
            generate_expression(gen, node);
            codegen_write(gen, ";\n");
    }
}

bool codegen_generate(CodeGenerator* gen, ASTNode* ast) {
    if (!ast) return false;
    
    codegen_generate_header(gen);
    
    // Generate global declarations
    for (ASTNode* node = ast->as.block.statements; node; node = node->next) {
        if (node->type == NODE_FUNCTION_DEF || node->type == NODE_VAR_DECL) {
            generate_statement(gen, node);
        }
    }
    
    // Generate main function
    codegen_write(gen, "\nint main() {\n");
    codegen_indent(gen);
    
    // Generate statements
    for (ASTNode* node = ast->as.block.statements; node; node = node->next) {
        if (node->type != NODE_FUNCTION_DEF && node->type != NODE_VAR_DECL) {
            generate_statement(gen, node);
        }
    }
    
    codegen_dedent(gen);
    codegen_write(gen, "    return 0;\n");
    codegen_write(gen, "}\n");
    
    return !gen->had_error;
} 