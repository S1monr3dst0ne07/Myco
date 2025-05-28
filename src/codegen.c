#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define myco_error(msg) do { fprintf(stderr, "Error: %s\n", msg); exit(1); } while(0)

static void indent(CodeGen *gen) {
    for (int i = 0; i < gen->indent_level; i++) {
        fprintf(gen->output, "    ");
    }
}

static void generate_expression(CodeGen *gen, ASTNode *node);
static void generate_statement(CodeGen *gen, ASTNode *node);

static void generate_literal(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL_NUMBER:
            fprintf(gen->output, "%g", node->as.number);
            break;
        case AST_LITERAL_STRING:
            fprintf(gen->output, "\"%s\"", node->as.string);
            break;
        case AST_LITERAL_BOOL:
            fprintf(gen->output, "%s", node->as.boolean ? "true" : "false");
            break;
        default:
            myco_error("Invalid literal type in code generation");
    }
}

static void generate_identifier(CodeGen *gen, ASTNode *node) {
    fprintf(gen->output, "%s", node->as.identifier);
}

static void generate_binary(CodeGen *gen, ASTNode *node) {
    fprintf(gen->output, "(");
    generate_expression(gen, node->as.binary.left);
    
    switch (node->as.binary.op) {
        case TOKEN_PLUS: fprintf(gen->output, " + "); break;
        case TOKEN_MINUS: fprintf(gen->output, " - "); break;
        case TOKEN_STAR: fprintf(gen->output, " * "); break;
        case TOKEN_SLASH: fprintf(gen->output, " / "); break;
        case TOKEN_PERCENT: fprintf(gen->output, " %% "); break;
        case TOKEN_EQ: fprintf(gen->output, " == "); break;
        case TOKEN_NEQ: fprintf(gen->output, " != "); break;
        case TOKEN_LT: fprintf(gen->output, " < "); break;
        case TOKEN_GT: fprintf(gen->output, " > "); break;
        case TOKEN_LTE: fprintf(gen->output, " <= "); break;
        case TOKEN_GTE: fprintf(gen->output, " >= "); break;
        case TOKEN_AND: fprintf(gen->output, " && "); break;
        case TOKEN_OR: fprintf(gen->output, " || "); break;
        case TOKEN_DOTDOT: fprintf(gen->output, " .. "); break;
        default: myco_error("Invalid binary operator in code generation");
    }
    
    generate_expression(gen, node->as.binary.right);
    fprintf(gen->output, ")");
}

static void generate_unary(CodeGen *gen, ASTNode *node) {
    switch (node->as.unary.op) {
        case TOKEN_MINUS: fprintf(gen->output, "-"); break;
        case TOKEN_NOT: fprintf(gen->output, "!"); break;
        default: myco_error("Invalid unary operator in code generation");
    }
    generate_expression(gen, node->as.unary.expr);
}

static void generate_call(CodeGen *gen, ASTNode *node) {
    generate_expression(gen, node->as.call.callee);
    fprintf(gen->output, "(");
    
    for (size_t i = 0; i < node->as.call.arg_count; i++) {
        if (i > 0) fprintf(gen->output, ", ");
        generate_expression(gen, node->as.call.args[i]);
    }
    
    fprintf(gen->output, ")");
}

static void generate_member(CodeGen *gen, ASTNode *node) {
    generate_expression(gen, node->as.member.object);
    fprintf(gen->output, ".%s", node->as.member.property);
}

static void generate_index(CodeGen *gen, ASTNode *node) {
    generate_expression(gen, node->as.index.array);
    fprintf(gen->output, "[");
    generate_expression(gen, node->as.index.index);
    fprintf(gen->output, "]");
}

static void generate_expression(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL_NUMBER:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOL:
            generate_literal(gen, node);
            break;
        case AST_IDENTIFIER:
            generate_identifier(gen, node);
            break;
        case AST_BINARY:
            generate_binary(gen, node);
            break;
        case AST_UNARY:
            generate_unary(gen, node);
            break;
        case AST_CALL:
            generate_call(gen, node);
            break;
        case AST_MEMBER:
            generate_member(gen, node);
            break;
        case AST_INDEX:
            generate_index(gen, node);
            break;
        default:
            myco_error("Invalid expression type in code generation");
    }
}

static void generate_var_decl(CodeGen *gen, ASTNode *node) {
    indent(gen);
    if (node->as.var_decl.is_const) {
        fprintf(gen->output, "const ");
    } else {
        fprintf(gen->output, "let ");
    }
    
    fprintf(gen->output, "%s", node->as.var_decl.name);
    
    if (node->as.var_decl.type != TOKEN_ERROR) {
        fprintf(gen->output, ": ");
        switch (node->as.var_decl.type) {
            case TOKEN_INT: fprintf(gen->output, "int"); break;
            case TOKEN_FLOAT: fprintf(gen->output, "float"); break;
            case TOKEN_STRING_TYPE: fprintf(gen->output, "string"); break;
            case TOKEN_BOOL: fprintf(gen->output, "bool"); break;
            default: myco_error("Invalid type in variable declaration");
        }
    }
    
    if (node->as.var_decl.initializer) {
        fprintf(gen->output, " = ");
        generate_expression(gen, node->as.var_decl.initializer);
    }
    
    fprintf(gen->output, "\n");
}

static void generate_if(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "if ");
    generate_expression(gen, node->as.if_stmt.condition);
    fprintf(gen->output, ":\n");
    
    gen->indent_level++;
    generate_statement(gen, node->as.if_stmt.then_branch);
    gen->indent_level--;
    
    if (node->as.if_stmt.else_branch) {
        indent(gen);
        fprintf(gen->output, "else:\n");
        gen->indent_level++;
        generate_statement(gen, node->as.if_stmt.else_branch);
        gen->indent_level--;
    }
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_while(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "while ");
    generate_expression(gen, node->as.while_stmt.condition);
    fprintf(gen->output, ":\n");
    
    gen->indent_level++;
    generate_statement(gen, node->as.while_stmt.body);
    gen->indent_level--;
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_for(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "for ");
    generate_statement(gen, node->as.for_stmt.init);
    fprintf(gen->output, "; ");
    generate_expression(gen, node->as.for_stmt.condition);
    fprintf(gen->output, "; ");
    generate_expression(gen, node->as.for_stmt.update);
    fprintf(gen->output, ":\n");
    
    gen->indent_level++;
    generate_statement(gen, node->as.for_stmt.body);
    gen->indent_level--;
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_for_in(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "for %s in ", node->as.for_in.var);
    generate_expression(gen, node->as.for_in.iterable);
    fprintf(gen->output, ":\n");
    
    gen->indent_level++;
    generate_statement(gen, node->as.for_in.body);
    gen->indent_level--;
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_function(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "func %s(", node->as.function.name);
    
    for (size_t i = 0; i < node->as.function.param_count; i++) {
        if (i > 0) fprintf(gen->output, ", ");
        fprintf(gen->output, "%s", node->as.function.params[i]);
        
        if (node->as.function.param_types[i] != TOKEN_ERROR) {
            fprintf(gen->output, ": ");
            switch (node->as.function.param_types[i]) {
                case TOKEN_INT: fprintf(gen->output, "int"); break;
                case TOKEN_FLOAT: fprintf(gen->output, "float"); break;
                case TOKEN_STRING_TYPE: fprintf(gen->output, "string"); break;
                case TOKEN_BOOL: fprintf(gen->output, "bool"); break;
                default: myco_error("Invalid parameter type in function declaration");
            }
        }
    }
    
    fprintf(gen->output, ")");
    
    if (node->as.function.return_type != TOKEN_ERROR) {
        fprintf(gen->output, " -> ");
        switch (node->as.function.return_type) {
            case TOKEN_INT: fprintf(gen->output, "int"); break;
            case TOKEN_FLOAT: fprintf(gen->output, "float"); break;
            case TOKEN_STRING_TYPE: fprintf(gen->output, "string"); break;
            case TOKEN_BOOL: fprintf(gen->output, "bool"); break;
            default: myco_error("Invalid return type in function declaration");
        }
    }
    
    fprintf(gen->output, ":\n");
    
    gen->indent_level++;
    generate_statement(gen, node->as.function.body);
    gen->indent_level--;
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_return(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "return");
    
    if (node->as.return_stmt.value) {
        fprintf(gen->output, " ");
        generate_expression(gen, node->as.return_stmt.value);
    }
    
    fprintf(gen->output, "\n");
}

static void generate_try(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "try:\n");
    
    gen->indent_level++;
    generate_statement(gen, node->as.try_stmt.try_block);
    gen->indent_level--;
    
    indent(gen);
    fprintf(gen->output, "catch %s:\n", node->as.try_stmt.catch_var);
    
    gen->indent_level++;
    generate_statement(gen, node->as.try_stmt.catch_block);
    gen->indent_level--;
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_switch(CodeGen *gen, ASTNode *node) {
    indent(gen);
    fprintf(gen->output, "switch ");
    generate_expression(gen, node->as.switch_stmt.value);
    fprintf(gen->output, ":\n");
    
    for (size_t i = 0; i < node->as.switch_stmt.case_count; i++) {
        ASTNode *case_node = node->as.switch_stmt.cases[i];
        
        indent(gen);
        if (case_node->as.case_stmt.condition) {
            fprintf(gen->output, "case ");
            generate_expression(gen, case_node->as.case_stmt.condition);
        } else {
            fprintf(gen->output, "default");
        }
        fprintf(gen->output, ":\n");
        
        gen->indent_level++;
        generate_statement(gen, case_node->as.case_stmt.body);
        gen->indent_level--;
    }
    
    indent(gen);
    fprintf(gen->output, "end\n");
}

static void generate_multi_assign(CodeGen *gen, ASTNode *node) {
    indent(gen);
    
    for (size_t i = 0; i < node->as.multi_assign.name_count; i++) {
        if (i > 0) fprintf(gen->output, ", ");
        fprintf(gen->output, "%s", node->as.multi_assign.names[i]);
    }
    
    fprintf(gen->output, " = ");
    
    for (size_t i = 0; i < node->as.multi_assign.value_count; i++) {
        if (i > 0) fprintf(gen->output, ", ");
        generate_expression(gen, node->as.multi_assign.values[i]);
    }
    
    fprintf(gen->output, "\n");
}

static void generate_statement(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case AST_VAR_DECL:
            generate_var_decl(gen, node);
            break;
        case AST_IF:
            generate_if(gen, node);
            break;
        case AST_WHILE:
            generate_while(gen, node);
            break;
        case AST_FOR:
            generate_for(gen, node);
            break;
        case AST_FOR_IN:
            generate_for_in(gen, node);
            break;
        case AST_FUNCTION:
            generate_function(gen, node);
            break;
        case AST_RETURN:
            generate_return(gen, node);
            break;
        case AST_TRY:
            generate_try(gen, node);
            break;
        case AST_SWITCH:
            generate_switch(gen, node);
            break;
        case AST_MULTI_ASSIGN:
            generate_multi_assign(gen, node);
            break;
        case AST_LITERAL_NUMBER:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOL:
        case AST_IDENTIFIER:
        case AST_BINARY:
        case AST_UNARY:
        case AST_CALL:
        case AST_MEMBER:
        case AST_INDEX:
            indent(gen);
            generate_expression(gen, node);
            fprintf(gen->output, "\n");
            break;
        default:
            myco_error("Invalid statement type in code generation");
    }
}

void codegen_init(CodeGen *gen, FILE *output) {
    gen->output = output;
    gen->indent_level = 0;
    gen->in_function = false;
    gen->current_function = NULL;
}

void codegen_generate(CodeGen *gen, ASTNode *node) {
    if (node->type != AST_PROGRAM) {
        myco_error("Expected program node in code generation");
    }
    
    for (size_t i = 0; i < node->as.program.statement_count; i++) {
        generate_statement(gen, node->as.program.statements[i]);
    }
}

void codegen_free(CodeGen *gen) {
    // Nothing to free
} 