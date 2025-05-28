#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "myco.h"

typedef enum {
    // Expressions
    AST_LITERAL_NUMBER,
    AST_LITERAL_STRING,
    AST_LITERAL_BOOL,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_UNARY,
    AST_CALL,
    AST_INDEX,
    AST_MEMBER,
    AST_ARRAY,
    
    // Statements
    AST_PROGRAM,
    AST_VAR_DECL,
    AST_CONST_DECL,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FOR_IN,
    AST_FUNCTION,
    AST_RETURN,
    AST_PRINT,
    AST_SWITCH,
    AST_CASE,
    AST_TRY,
    AST_MULTI_ASSIGN
} ASTNodeType;

typedef struct ASTNode ASTNode;

typedef struct {
    ASTNode *left;
    TokenType op;
    ASTNode *right;
} BinaryExpr;

typedef struct {
    TokenType op;
    ASTNode *expr;
} UnaryExpr;

typedef struct {
    ASTNode *callee;
    ASTNode **args;
    size_t arg_count;
} CallExpr;

typedef struct {
    ASTNode *array;
    ASTNode *index;
} IndexExpr;

typedef struct {
    ASTNode *object;
    char *property;
} MemberExpr;

typedef struct {
    ASTNode **elements;
    size_t element_count;
} ArrayExpr;

typedef struct {
    char *name;
    bool is_const;
    TokenType type;
    ASTNode *initializer;
} VarDecl;

typedef struct {
    ASTNode *condition;
    ASTNode *then_branch;
    ASTNode *else_branch;
} IfStmt;

typedef struct {
    ASTNode *init;
    ASTNode *condition;
    ASTNode *update;
    ASTNode *body;
} ForStmt;

typedef struct {
    char *var;
    ASTNode *iterable;
    ASTNode *body;
} ForInStmt;

typedef struct {
    char *name;
    char **params;
    size_t param_count;
    TokenType *param_types;
    TokenType return_type;
    ASTNode *body;
} FunctionStmt;

typedef struct {
    ASTNode *value;
} ReturnStmt;

typedef struct {
    ASTNode **args;
    size_t arg_count;
} PrintStmt;

typedef struct {
    ASTNode *value;
    ASTNode **cases;
    size_t case_count;
} SwitchStmt;

typedef struct {
    ASTNode *condition;
    ASTNode *body;
} CaseStmt;

typedef struct {
    ASTNode *try_block;
    char *catch_var;
    ASTNode *catch_block;
} TryStmt;

typedef struct {
    ASTNode **statements;
    size_t statement_count;
} ProgramStmt;

typedef struct {
    ASTNode *condition;
    ASTNode *body;
} WhileStmt;

typedef struct {
    char **names;
    size_t name_count;
    ASTNode **values;
    size_t value_count;
} MultiAssignStmt;

struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    union {
        double number;
        char *string;
        bool boolean;
        char *identifier;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
        IndexExpr index;
        MemberExpr member;
        ArrayExpr array;
        VarDecl var_decl;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        ForInStmt for_in;
        FunctionStmt function;
        ReturnStmt return_stmt;
        PrintStmt print_stmt;
        SwitchStmt switch_stmt;
        CaseStmt case_stmt;
        TryStmt try_stmt;
        ProgramStmt program;
        MultiAssignStmt multi_assign;
    } as;
};

// Constructor functions
ASTNode *ast_literal_number(double value, int line, int column);
ASTNode *ast_literal_string(const char *value, int line, int column);
ASTNode *ast_literal_bool(bool value, int line, int column);
ASTNode *ast_identifier(const char *name, int line, int column);
ASTNode *ast_binary(ASTNode *left, TokenType op, ASTNode *right, int line, int column);
ASTNode *ast_unary(TokenType op, ASTNode *expr, int line, int column);
ASTNode *ast_call(ASTNode *callee, ASTNode **args, size_t arg_count, int line, int column);
ASTNode *ast_index(ASTNode *array, ASTNode *index, int line, int column);
ASTNode *ast_member(ASTNode *object, const char *property, int line, int column);
ASTNode *ast_array(ASTNode **elements, size_t element_count, int line, int column);
ASTNode *ast_var_decl(const char *name, bool is_const, ASTNode *init, int line, int column);
ASTNode *ast_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch, int line, int column);
ASTNode *ast_while(ASTNode *condition, ASTNode *body, int line, int column);
ASTNode *ast_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body, int line, int column);
ASTNode *ast_for_in(const char *var, ASTNode *iterable, ASTNode *body, int line, int column);
ASTNode *ast_function(const char *name, char **params, size_t param_count, ASTNode *body, int line, int column);
ASTNode *ast_return(ASTNode *value, int line, int column);
ASTNode *ast_print(ASTNode **args, size_t arg_count, int line, int column);
ASTNode *ast_switch(ASTNode *value, ASTNode **cases, size_t case_count, int line, int column);
ASTNode *ast_case(ASTNode *condition, ASTNode *body, int line, int column);
ASTNode *ast_try(ASTNode *try_block, const char *catch_var, ASTNode *catch_block, int line, int column);
ASTNode *ast_program(ASTNode **statements, size_t statement_count);

// New constructor functions
ASTNode *ast_var_decl_with_type(const char *name, bool is_const, TokenType type, ASTNode *init, int line, int column);
ASTNode *ast_function_with_types(const char *name, char **params, TokenType *param_types, size_t param_count, TokenType return_type, ASTNode *body, int line, int column);
ASTNode *ast_multi_assign(char **names, size_t name_count, ASTNode **values, size_t value_count, int line, int column);

// Memory management
void ast_free(ASTNode *node);

#endif // AST_H 