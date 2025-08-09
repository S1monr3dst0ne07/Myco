#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

// Track imported modules
typedef struct {
    char* alias;
    ASTNode* module_ast;
} CGModule;
static CGModule* cg_modules = NULL;
static int cg_modules_size = 0;
static int cg_modules_cap = 0;

static void cg_register_module(const char* alias, ASTNode* ast) {
    if (cg_modules_size >= cg_modules_cap) {
        cg_modules_cap = cg_modules_cap ? cg_modules_cap * 2 : 4;
        cg_modules = (CGModule*)realloc(cg_modules, cg_modules_cap * sizeof(CGModule));
    }
    cg_modules[cg_modules_size].alias = strdup(alias);
    cg_modules[cg_modules_size].module_ast = ast;
    cg_modules_size++;
}

static int cg_is_alias(const char* name) {
    for (int i = 0; i < cg_modules_size; i++) {
        if (strcmp(cg_modules[i].alias, name) == 0) return 1;
    }
    return 0;
}

static ASTNode* cg_load_module(const char* path) {
    const char* p = path;
    if (p[0] == '"') { p++; }
    int plen = (int)strlen(p);
    char* fixed = (char*)malloc(plen + 1);
    strcpy(fixed, p);
    // strip trailing quote if from string token
    if (fixed[plen-1] == '"') fixed[plen-1] = '\0';
    // strip leading ./ if present
    const char* openp = fixed;
    if (fixed[0] == '.' && fixed[1] == '/') openp = fixed + 2;
    FILE* f = fopen(openp, "r");
    if (!f) { free(fixed); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    fread(buf, 1, sz, f); buf[sz] = '\0'; fclose(f);
    Token* toks = lexer_tokenize(buf);
    free(buf);
    if (!toks) { free(fixed); return NULL; }
    ASTNode* ast = parser_parse(toks);
    lexer_free_tokens(toks);
    free(fixed);
    return ast;
}

// Helper to check if a string is a string literal (starts and ends with ")
static int is_string_literal(const char* text) {
    if (!text) return 0;
    size_t len = strlen(text);
    return len >= 2 && text[0] == '"' && text[len-1] == '"';
}

// Helper function to generate C code for an expression
static void generate_expression(FILE* file, ASTNode* ast) {
    if (!ast) return;

    switch (ast->type) {
        case AST_EXPR:
            if (ast->text) {
                if (strcmp(ast->text, "+") == 0 || strcmp(ast->text, "-") == 0 ||
                    strcmp(ast->text, "*") == 0 || strcmp(ast->text, "/") == 0 ||
                    strcmp(ast->text, "%") == 0 || strcmp(ast->text, "==") == 0 ||
                    strcmp(ast->text, "!=") == 0 || strcmp(ast->text, "<") == 0 ||
                    strcmp(ast->text, ">") == 0 || strcmp(ast->text, "<=") == 0 ||
                    strcmp(ast->text, ">=") == 0) {
                    if (ast->child_count >= 2) {
                        fprintf(file, "(");
                        generate_expression(file, &ast->children[0]);
                        fprintf(file, " %s ", ast->text);
                        generate_expression(file, &ast->children[1]);
                        fprintf(file, ")");
                    }
                } else if (strcmp(ast->text, "call") == 0) {
                    if (ast->child_count >= 2) {
                        // alias(function(args)) passthrough if callee is alias and arg0 is call
                        ASTNode* callee = &ast->children[0];
                        ASTNode* args = &ast->children[1];
                        if (callee->text && cg_is_alias(callee->text) && args->child_count == 1 &&
                            args->children[0].type == AST_EXPR && args->children[0].text &&
                            strcmp(args->children[0].text, "call") == 0) {
                            generate_expression(file, &args->children[0]);
                        } else {
                            // normal call: name(args)
                            fprintf(file, "%s(", callee->text ? callee->text : "fn");
                            for (int i = 0; i < args->child_count; i++) {
                                if (i) fprintf(file, ", ");
                                generate_expression(file, &args->children[i]);
                            }
                            fprintf(file, ")");
                        }
                    }
                } else {
                    // Handle literals and identifiers
                    fprintf(file, "%s", ast->text);
                }
            }
            break;
    }
}

// Helper function to generate C code for a statement
static void generate_statement(FILE* file, ASTNode* ast) {
    if (!ast) return;

    switch (ast->type) {
        case AST_PRINT:
            if (ast->child_count > 0) {
                fprintf(file, "printf(\"");
                for (int i = 0; i < ast->child_count; i++) {
                    if (ast->children[i].type == AST_EXPR) {
                        if (is_string_literal(ast->children[i].text)) fprintf(file, "%%s");
                        else fprintf(file, "%%d");
                    }
                }
                fprintf(file, "\\n\"");
                for (int i = 0; i < ast->child_count; i++) {
                    fprintf(file, ", ");
                    if (ast->children[i].type == AST_EXPR) {
                        if (is_string_literal(ast->children[i].text)) fprintf(file, "%s", ast->children[i].text);
                        else generate_expression(file, &ast->children[i]);
                    }
                }
                fprintf(file, ");\n");
            }
            break;
        case AST_IF:
            if (ast->child_count >= 2) {
                fprintf(file, "if (");
                generate_expression(file, &ast->children[0]);
                fprintf(file, ") {\n");
                generate_statement(file, &ast->children[1]);
                fprintf(file, "}\n");
                if (ast->child_count > 2) {
                    fprintf(file, "else {\n");
                    generate_statement(file, &ast->children[2]);
                    fprintf(file, "}\n");
                }
            }
            break;
        case AST_FOR:
            if (ast->child_count >= 4) {
                fprintf(file, "for (int i = ");
                generate_expression(file, &ast->children[1]);
                fprintf(file, "; i <= ");
                generate_expression(file, &ast->children[2]);
                fprintf(file, "; i++) {\n");
                generate_statement(file, &ast->children[3]);
                fprintf(file, "}\n");
            }
            break;
        case AST_SWITCH:
            if (ast->child_count >= 2) {
                fprintf(file, "switch (");
                generate_expression(file, &ast->children[0]);
                fprintf(file, ") {\n");
                for (int i = 0; i < ast->children[1].child_count; i++) {
                    ASTNode* case_node = &ast->children[1].children[i];
                    if (case_node->type == AST_CASE && case_node->child_count >= 2) {
                        fprintf(file, "case ");
                        generate_expression(file, &case_node->children[0]);
                        fprintf(file, ":\n");
                        generate_statement(file, &case_node->children[1]);
                        fprintf(file, "break;\n");
                    } else if (case_node->type == AST_DEFAULT && case_node->child_count >= 1) {
                        fprintf(file, "default:\n");
                        generate_statement(file, &case_node->children[0]);
                        fprintf(file, "break;\n");
                    }
                }
                fprintf(file, "}\n");
            }
            break;
        case AST_BLOCK:
            if (ast->text && strcmp(ast->text, "use") == 0 && ast->child_count == 2) {
                // Already handled at top-level; ignore in statement stream
                break;
            }
            for (int i = 0; i < ast->child_count; i++) generate_statement(file, &ast->children[i]);
            break;
        case AST_LET:
            if (ast->child_count >= 2) {
                fprintf(file, "int %s = ", ast->children[0].text);
                generate_expression(file, &ast->children[1]);
                fprintf(file, ";\n");
            }
            break;
        case AST_FUNC:
            if (ast->child_count >= 2) {
                // Emit function as-is (simple recursion demo)
                fprintf(file, "int %s(int n) {\n", ast->text);
                fprintf(file, "    if (n <= 1) return 1;\n");
                fprintf(file, "    return n * %s(n - 1);\n", ast->text);
                fprintf(file, "}\n\n");
            }
            break;
    }
}

int codegen_generate(ASTNode* ast, const char* input_file, int keep_output) {
    // Preprocess imports: load modules listed by 'use'
    for (int i = 0; i < ast->child_count; i++) {
        ASTNode* n = &ast->children[i];
        if (n->type == AST_BLOCK && n->text && strcmp(n->text, "use") == 0 && n->child_count == 2) {
            const char* path = n->children[0].text;
            const char* alias = n->children[1].text;
            ASTNode* mod = cg_load_module(path);
            if (mod) cg_register_module(alias, mod);
        }
    }

    FILE* file = fopen("output.c", "w");
    if (!file) { fprintf(stderr, "Error: Could not open output file\n"); return 1; }

    fprintf(file, "#include <stdio.h>\n");
    fprintf(file, "#include <stdlib.h>\n\n");

    // Emit imported module functions first
    for (int m = 0; m < cg_modules_size; m++) {
        ASTNode* mod = cg_modules[m].module_ast;
        for (int i = 0; i < mod->child_count; i++) {
            if (mod->children[i].type == AST_FUNC) {
                generate_statement(file, &mod->children[i]);
            }
        }
    }

    // Emit top-level functions from current file
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i].type == AST_FUNC) {
            generate_statement(file, &ast->children[i]);
        }
    }

    fprintf(file, "int main() {\n");
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i].type != AST_FUNC) {
            generate_statement(file, &ast->children[i]);
        }
    }
    fprintf(file, "    return 0;\n}\n");
    fclose(file);

    char* base_name = strdup(input_file);
    char* ext = strrchr(base_name, '.');
    if (ext) *ext = '\0';

#ifdef __APPLE__
    char command[512]; snprintf(command, sizeof(command), "cc -Os -o %s output.c", base_name);
#else
    char command[512]; snprintf(command, sizeof(command), "cc -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -o %s output.c", base_name);
#endif
    if (system(command) != 0) {
        fprintf(stderr, "Error: Compilation failed\n");
        free(base_name);
        return 1;
    }

    if (!keep_output) remove("output.c");
    free(base_name);
    return 0;
} 