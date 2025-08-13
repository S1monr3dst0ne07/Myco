#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "memory_tracker.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/select.h>
#endif

// Forward declarations for functions defined later in this file
static struct ASTNode* find_function_in_module(struct ASTNode* mod, const char* name);
long long eval_expression(struct ASTNode* ast);
void eval_evaluate(struct ASTNode* ast);
static struct ASTNode* resolve_module(const char* alias);
static long long eval_user_function_call(struct ASTNode* fn, struct ASTNode* args_node);
static const char* get_str_value(const char* name);

// Add new function for finding functions with module prefix
static ASTNode* find_function_with_module_prefix(const char* module_name, const char* function_name);
static ASTNode* find_function_in_any_module(const char* name);

// ANSI color codes
#define RED "\033[31m"
#define RESET "\033[0m"

// Error code structure: 0xMY[SEV][MOD][ERR]
// SEV: Severity (0=Info, 1=Warning, 2=Error, F=Fatal)
// MOD: Module (0=Runtime, 1=Math, 2=Type, 3=Syntax, 4=IO)
// ERR: Specific error code

// Severity levels
#define SEV_INFO    0x00
#define SEV_WARNING 0x01
#define SEV_ERROR   0x02
#define SEV_FATAL   0x0F

// Modules
#define MOD_RUNTIME 0x00
#define MOD_MATH    0x01
#define MOD_TYPE    0x02
#define MOD_SYNTAX  0x03
#define MOD_IO      0x04

// Specific error codes
#define ERR_NONE             0x00
#define ERR_DIVISION_BY_ZERO 0x01
#define ERR_MODULO_BY_ZERO   0x02
#define ERR_UNDEFINED_VAR    0x03
#define ERR_TYPE_MISMATCH    0x04
#define ERR_INVALID_OP       0x05
#define ERR_RECURSION        0x06
#define ERR_FUNC_CALL        0x07
#define ERR_BAD_MEMORY       0x08
#define ERR_INPUT_FAILED     0x09
#define ERR_INVALID_INPUT    0x0A

// Combined error codes
#define ERROR_DIVISION_BY_ZERO   ((SEV_ERROR << 16) | (MOD_MATH << 8) | ERR_DIVISION_BY_ZERO)
#define ERROR_MODULO_BY_ZERO     ((SEV_ERROR << 16) | (MOD_MATH << 8) | ERR_MODULO_BY_ZERO)
#define ERROR_UNDEFINED_VAR      ((SEV_ERROR << 16) | (MOD_RUNTIME << 8) | ERR_UNDEFINED_VAR)
#define ERROR_TYPE_MISMATCH      ((SEV_ERROR << 16) | (MOD_TYPE << 8) | ERR_TYPE_MISMATCH)
#define ERROR_INVALID_OP         ((SEV_ERROR << 16) | (MOD_RUNTIME << 8) | ERR_INVALID_OP)
#define ERROR_RECURSION          ((SEV_ERROR << 16) | (MOD_RUNTIME << 8) | ERR_RECURSION)
#define ERROR_FUNC_CALL          ((SEV_ERROR << 16) | (MOD_RUNTIME << 8) | ERR_FUNC_CALL)
#define ERROR_BAD_MEMORY         ((SEV_FATAL << 16) | (MOD_RUNTIME << 8) | ERR_BAD_MEMORY)
#define ERROR_INPUT_FAILED       ((SEV_ERROR << 16) | (MOD_IO << 8) | ERR_INPUT_FAILED)
#define ERROR_INVALID_INPUT      ((SEV_ERROR << 16) | (MOD_IO << 8) | ERR_INVALID_INPUT)

// Error messages
static const char* error_messages[] = {
    "No error",
    "Division by zero",
    "Modulo by zero",
    "Undefined variable",
    "Type mismatch",
    "Invalid operation",
    "Recursion error",
    "Function call error",
    "Bad memory access"
};

// Helper to check if a string is a string literal (starts and ends with ")
static int is_string_literal(const char* text) {
    if (!text) return 0;
    size_t len = strlen(text);
    return len >= 2 && text[0] == '"' && text[len-1] == '"';
}

// Helper function to check if a variable exists
int var_exists(const char* name);

// Helper function to check if a node represents a string
static int is_string_node(ASTNode* node) {
    if (!node) return 0;
    
    // If it's a string literal (quoted text), it's a string
    if (node->type == AST_EXPR && node->text && is_string_literal(node->text)) {
        return 1;
    }
    
    // If it's a variable, check if it's a string variable
    if (node->type == AST_EXPR && node->text) {
        // First check if it's a numeric variable
        if (var_exists(node->text)) {
            return 0; // It's a numeric variable, not a string
        }
        // Then check if it's a string variable
        const char* str_val = get_str_value(node->text);
        if (str_val) {
            return 1;
        }
    }
    
    // Everything else is not a string
    return 0;
}

// Simple variable environment: dynamic array of variable names and values
typedef struct {
    char* name;
    long long value;
} VarEntry;

static VarEntry* var_env = NULL;
static int var_env_size = 0;
static int var_env_capacity = 0;

// String variable environment
typedef struct {
    char* name;
    char* value;
} StrEntry;
static StrEntry* str_env = NULL;
static int str_env_size = 0;
static int str_env_capacity = 0;

// Simple module alias mapping
typedef struct {
    char* alias;
    ASTNode* module_ast;
} ModuleEntry;
static ModuleEntry* modules = NULL;
static int modules_size = 0;
static int modules_cap = 0;

// Base directory for resolving relative module paths
static char base_dir[1024] = "";
void eval_set_base_dir(const char* dir) {
    if (!dir) { base_dir[0] = '\0'; return; }
    size_t n = strlen(dir);
    if (n >= sizeof(base_dir)) n = sizeof(base_dir) - 1;
    strncpy(base_dir, dir, n);
    base_dir[n] = '\0';
}

static void compute_full_path(const char* path, char* out, size_t out_size) {
    const char* rel = path;
    if (rel[0] == '.' && rel[1] == '/') rel = rel + 2;
    
    // Add .myco extension if it's missing
    char path_with_ext[1024];
    if (strlen(rel) > 5 && strcmp(rel + strlen(rel) - 5, ".myco") == 0) {
        // Already has .myco extension
        strncpy(path_with_ext, rel, sizeof(path_with_ext) - 1);
        path_with_ext[sizeof(path_with_ext) - 1] = '\0';
    } else {
        // Add .myco extension
        snprintf(path_with_ext, sizeof(path_with_ext), "%s.myco", rel);
    }
    
    if (base_dir[0]) snprintf(out, out_size, "%s/%s", base_dir, path_with_ext);
    else snprintf(out, out_size, "%s", path_with_ext);
}

// Global function registry
typedef struct {
    char* name;
    ASTNode* func_ast; // points to AST_FUNC node
} FuncEntry;
static FuncEntry* functions = NULL;
static int functions_size = 0;
static int functions_cap = 0;

// Discord Gateway minimal state
#ifndef _WIN32
static int gw_in_fd = -1;
static int gw_out_fd = -1;
static FILE* gw_in = NULL;
static FILE* gw_out = NULL;
#endif
static int gw_seq = -1;
static int gw_heartbeat_ms = 0;

#ifndef _WIN32
static int file_executable(const char* p) {
    return (p && access(p, X_OK) == 0);
}

static int ensure_dir(const char* path) {
    struct stat st; if (stat(path, &st) == 0) return 0; return mkdir(path, 0700);
}

static int ensure_websocat(char* out_path, size_t out_sz) {
    const char* env = getenv("MYCO_WEBSOCAT");
    if (env && env[0] && file_executable(env)) { snprintf(out_path, out_sz, "%s", env); return 0; }

    // Try local .myco/bin/websocat
    const char* rel_dir = ".myco/bin";
    (void)ensure_dir(".myco"); (void)ensure_dir(rel_dir);
    char local_bin[512]; snprintf(local_bin, sizeof(local_bin), "%s/websocat", rel_dir);
    if (file_executable(local_bin)) { snprintf(out_path, out_sz, "%s", local_bin); return 0; }

    // Determine OS/arch for download URL
    struct utsname un; uname(&un);
    const char* url = NULL;
    // Default to macOS x86_64
    if (strcmp(un.sysname, "Darwin") == 0) {
        if (strcmp(un.machine, "arm64") == 0 || strcmp(un.machine, "aarch64") == 0) {
            url = "https://github.com/vi/websocat/releases/download/v1.12.0/websocat_macos_arm64";
        } else {
            url = "https://github.com/vi/websocat/releases/download/v1.12.0/websocat_macos";
        }
    } else {
        // Assume Linux amd64
        url = "https://github.com/vi/websocat/releases/download/v1.12.0/websocat_amd64-linux";
    }
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -L -sS -o %s %s && chmod +x %s", local_bin, url, local_bin);
    int rc = system(cmd);
    if (rc == 0 && file_executable(local_bin)) { snprintf(out_path, out_sz, "%s", local_bin); return 0; }
    return -1;
}

static int gateway_spawn_websocat(const char* url) {
    int inpipe[2];
    int outpipe[2];
    if (pipe(inpipe) != 0) return -1;
    if (pipe(outpipe) != 0) { close(inpipe[0]); close(inpipe[1]); return -1; }
    pid_t pid = fork();
    if (pid < 0) { close(inpipe[0]); close(inpipe[1]); close(outpipe[0]); close(outpipe[1]); return -1; }
    if (pid == 0) {
        // child
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);
        close(inpipe[1]); close(outpipe[0]);
        close(inpipe[0]); close(outpipe[1]);
        char binbuf[512]; const char* bin = getenv("MYCO_WEBSOCAT");
        if (!bin || !bin[0]) {
            if (ensure_websocat(binbuf, sizeof(binbuf)) == 0) bin = binbuf; else bin = "websocat";
        }
        execlp(bin, bin, "-t", url, (char*)NULL);
        _exit(127);
    }
    // parent
    close(inpipe[0]); close(outpipe[1]);
    gw_in_fd = inpipe[1];
    gw_out_fd = outpipe[0];
    gw_in = fdopen(gw_in_fd, "w");
    gw_out = fdopen(gw_out_fd, "r");
    setvbuf(gw_in, NULL, _IONBF, 0);
    setvbuf(gw_out, NULL, _IONBF, 0);
    return 0;
}
#endif

static void register_function(const char* name, ASTNode* fn) {
    if (!name || !fn) return;
    
    // Check if function already exists
    for (int i = functions_size - 1; i >= 0; i--) {
        if (strcmp(functions[i].name, name) == 0) {
            // Function already exists, update it
            functions[i].func_ast = fn;
            return;
        }
    }
    
    // Expand capacity if needed
    if (functions_size >= functions_cap) {
        int new_cap = functions_cap ? functions_cap * 2 : 8;
        FuncEntry* new_funcs = (FuncEntry*)realloc(functions, new_cap * sizeof(FuncEntry));
        if (!new_funcs) {
            // Handle realloc failure
            return;
        }
        functions = new_funcs;
        functions_cap = new_cap;
    }
    
    // Add new function
    functions[functions_size].name = strdup(name);
    if (functions[functions_size].name) {
        functions[functions_size].func_ast = fn;
        functions_size++;
    }
}

static ASTNode* find_function_global(const char* name) {
    if (!name) return NULL;
    for (int i = functions_size - 1; i >= 0; i--) {
        if (strcmp(functions[i].name, name) == 0) return functions[i].func_ast;
    }
    // fallback: search modules directly
    for (int mi = 0; mi < modules_size; mi++) {
        ASTNode* fn = find_function_in_module(modules[mi].module_ast, name);
        if (fn) return fn;
    }
    return NULL;
}

// Error handling state
static int in_try_block = 0;
static int in_catch_block = 0; // NEW: track when evaluating catch body
static int error_occurred = 0;
static int error_value = 0;
static int error_printed = 0;

// Function return handling
static int return_flag = 0;
static long long return_value = 0;

// Global variables for tracking state
static int loop_counter = 0;

// Add global variable for current line
static int current_line = 1;

// Helper function to get error description
static const char* get_error_description(int error_code) {
    // Map error codes to descriptions
    switch (error_code) {
        case ERROR_DIVISION_BY_ZERO: return "division by zero";
        case ERROR_MODULO_BY_ZERO:   return "modulo by zero";
        case ERROR_UNDEFINED_VAR:    return "undefined variable";
        case ERROR_TYPE_MISMATCH:    return "type mismatch";
        case ERROR_INVALID_OP:       return "invalid operation";
        case ERROR_RECURSION:        return "recursion error";
        case ERROR_FUNC_CALL:        return "function call error";
        case ERROR_BAD_MEMORY:       return "bad memory access";
        case ERROR_INPUT_FAILED:     return "input failed";
        case ERROR_INVALID_INPUT:    return "invalid input";
        default:                     return "unknown error";
    }
}

// Helper function to format error message with proper error code
static void format_error_message(int error_code, int line, char* buffer, size_t size) {
    if (in_catch_block) {
        // For catch blocks: just the description
        snprintf(buffer, size, "%s%s%s", RED, get_error_description(error_code), RESET);
    } else {
        // Regular runtime report: include line
        const char* desc = get_error_description(error_code);
        char cap[256];
        strncpy(cap, desc, sizeof(cap) - 1);
        cap[sizeof(cap) - 1] = '\0';
        if (cap[0]) cap[0] = (char)toupper((unsigned char)cap[0]);
        snprintf(buffer, size, "%sLine %d: %s%s", RED, line, cap, RESET);
    }
}

// Helper function to set error state
static void set_error(int error_code) {
    error_occurred = 1;
    error_value = error_code;
    if (!in_try_block && !error_printed) {
        char error_msg[256];
        format_error_message(error_code, current_line, error_msg, sizeof(error_msg));
        fprintf(stderr, "%s\n", error_msg);
        error_printed = 1;
    }
}

// Helper function to handle error in catch block
static void handle_catch_error(int error_code) {
    char error_msg[256];
    format_error_message(error_code, current_line, error_msg, sizeof(error_msg));
    printf("%s\n", error_msg);
}

// Helper function to reset error state
static void reset_error_state() {
    error_occurred = 0;
    error_value = 0;
    error_printed = 0;
}

// Helper function to get a variable's value from the environment
static long long get_var_value(const char* name) {
    // Special case for loop counter
    if (strcmp(name, "i") == 0) {
        return loop_counter;
    }
    
    // Special case for error variable in catch block
    if (in_try_block && error_occurred && strcmp(name, "err") == 0) {
        return error_value;
    }
    
    // Check if variable exists
    for (int i = var_env_size - 1; i >= 0; i--) {
        if (strcmp(var_env[i].name, name) == 0) {
            return var_env[i].value;
        }
    }
    
    // If variable not found, return undefined variable error
    set_error(ERROR_UNDEFINED_VAR);
    return ERROR_UNDEFINED_VAR;
}

// Helper function to set a variable's value in the environment
static void set_var_value(const char* name, long long value) {
    // Check if variable already exists
    for (int i = var_env_size - 1; i >= 0; i--) {
        if (strcmp(var_env[i].name, name) == 0) {
            var_env[i].value = value;
            return;
        }
    }
    
    // Expand capacity if needed
    if (var_env_size >= var_env_capacity) {
        int new_capacity = var_env_capacity ? var_env_capacity * 2 : 8;
        VarEntry* new_env = (VarEntry*)realloc(var_env, new_capacity * sizeof(VarEntry));
        if (!new_env) {
            // Handle realloc failure
            return;
        }
        var_env = new_env;
        var_env_capacity = new_capacity;
    }
    
    // Add new variable
    var_env[var_env_size].name = strdup(name);
    if (var_env[var_env_size].name) {
        var_env[var_env_size].value = value;
        var_env_size++;
    }
}

// Helper function to check if a variable exists
int var_exists(const char* name) {
    for (int i = 0; i < var_env_size; i++) {
        if (strcmp(var_env[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static const char* get_str_value(const char* name) {
    for (int i = str_env_size - 1; i >= 0; i--) {
        if (strcmp(str_env[i].name, name) == 0) return str_env[i].value;
    }
    return NULL;
}

static void set_str_value(const char* name, const char* value) {
    // Safety check: ensure name is not NULL
    if (!name) {
        return;
    }
    
    // Check if variable already exists
    for (int i = str_env_size - 1; i >= 0; i--) {
        if (strcmp(str_env[i].name, name) == 0) {
            // Update existing variable
            if (str_env[i].value) {
                free(str_env[i].value);
                str_env[i].value = NULL; // Prevent double-free
            }
            // Create a safe copy of the value
            const char* safe_value = value ? value : "";
            str_env[i].value = strdup(safe_value);
            if (!str_env[i].value) {
                // Handle strdup failure
                str_env[i].value = strdup("");
            }
            return;
        }
    }
    
    // Expand capacity if needed
    if (str_env_size >= str_env_capacity) {
        int new_capacity = str_env_capacity ? str_env_capacity * 2 : 8;
        StrEntry* new_env = (StrEntry*)tracked_realloc(str_env, new_capacity * sizeof(StrEntry), __FILE__, __LINE__, "set_str_value");
        if (!new_env) {
            // Handle realloc failure
            return;
        }
        str_env = new_env;
        str_env_capacity = new_capacity;
    }
    
    // Add new variable
    str_env[str_env_size].name = strdup(name);
    str_env[str_env_size].value = strdup(value ? value : "");
    if (str_env[str_env_size].name && str_env[str_env_size].value) {
        str_env_size++;
    } else {
        // Handle allocation failure
        if (str_env[str_env_size].name) {
            tracked_free(str_env[str_env_size].name, __FILE__, __LINE__, "set_str_value");
            str_env[str_env_size].name = NULL;
        }
        if (str_env[str_env_size].value) {
            tracked_free(str_env[str_env_size].value, __FILE__, __LINE__, "set_str_value");
            str_env[str_env_size].value = NULL;
        }
    }
}

// Cleanup function for string environment
static void cleanup_str_env() {
    if (str_env && str_env_size > 0) {
        for (int i = 0; i < str_env_size; i++) {
            if (str_env[i].name) {
                tracked_free(str_env[i].name, __FILE__, __LINE__, "cleanup_str_env");
                str_env[i].name = NULL;
            }
            if (str_env[i].value) {
                tracked_free(str_env[i].value, __FILE__, __LINE__, "cleanup_str_env");
                str_env[i].value = NULL;
            }
        }
        tracked_free(str_env, __FILE__, __LINE__, "cleanup_str_env");
        str_env = NULL;
        str_env_size = 0;
        str_env_capacity = 0;
    }
}

// Cleanup function for variable environment
static void cleanup_var_env() {
    if (var_env && var_env_size > 0) {
        for (int i = 0; i < var_env_size; i++) {
                    if (var_env[i].name) {
            tracked_free(var_env[i].name, __FILE__, __LINE__, "cleanup_var_env");
            var_env[i].name = NULL;
        }
    }
    tracked_free(var_env, __FILE__, __LINE__, "cleanup_var_env");
    var_env = NULL;
    var_env_size = 0;
    var_env_capacity = 0;
    }
}

// Cleanup function for function environment
static void cleanup_func_env() {
    if (functions && functions_size > 0) {
        for (int i = 0; i < functions_size; i++) {
            if (functions[i].name) {
                tracked_free(functions[i].name, __FILE__, __LINE__, "cleanup_func_env");
                functions[i].name = NULL;
            }
        }
        tracked_free(functions, __FILE__, __LINE__, "cleanup_func_env");
        functions = NULL;
        functions_size = 0;
        functions_cap = 0;
    }
}

// Cleanup function for module environment
static void cleanup_module_env() {
    if (modules && modules_size > 0) {
        for (int i = 0; i < modules_size; i++) {
            if (modules[i].alias) {
                tracked_free(modules[i].alias, __FILE__, __LINE__, "cleanup_module_env");
                modules[i].alias = NULL;
            }
            // Don't free module_ast here as it's owned by the parser
            modules[i].module_ast = NULL;
        }
        tracked_free(modules, __FILE__, __LINE__, "cleanup_module_env");
        modules = NULL;
        modules_size = 0;
        modules_cap = 0;
    }
}

// Master cleanup function
void cleanup_all_environments() {
    cleanup_str_env();
    cleanup_var_env();
    cleanup_func_env();
    cleanup_module_env();
}

// Helper: is this integer an error code of our 0xMY[SEV][MOD][ERR] scheme?
static int is_error_code(int value) {
    int sev = (value >> 16) & 0xFF;
    return sev == 0x02 || sev == 0x0F; // ERROR or FATAL
}

// Helper function to format error value for printing
static void format_error_value(int value, char* buffer, size_t size) {
    if (is_error_code(value)) {
        const char* desc = get_error_description(value);
        char lower_desc[256];
        strncpy(lower_desc, desc, sizeof(lower_desc) - 1);
        lower_desc[0] = tolower(lower_desc[0]);
        snprintf(buffer, size, "%s%s%s", RED, lower_desc, RESET);
    } else {
        snprintf(buffer, size, "%d", value);
    }
}

static ASTNode* load_and_parse_module(const char* path) {
    char full[2048];
    compute_full_path(path, full, sizeof(full));
    FILE* f = fopen(full, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f); buf[sz] = '\0'; fclose(f);
    Token* toks = lexer_tokenize(buf);
    free(buf);
    if (!toks) return NULL;
    ASTNode* mod = parser_parse(toks);
    lexer_free_tokens(toks);
    return mod;
}

static void register_module(const char* alias, ASTNode* ast) {
    if (!alias) return;
    
    // Check if module already exists
    for (int i = 0; i < modules_size; i++) {
        if (strcmp(modules[i].alias, alias) == 0) {
            // Module already exists, update it
            modules[i].module_ast = ast;
            return;
        }
    }
    
    // Expand capacity if needed
    if (modules_size >= modules_cap) {
        int new_cap = modules_cap ? modules_cap * 2 : 4;
        ModuleEntry* new_modules = (ModuleEntry*)realloc(modules, new_cap * sizeof(ModuleEntry));
        if (!new_modules) {
            // Handle realloc failure
            return;
        }
        modules = new_modules;
        modules_cap = new_cap;
    }
    
    // Add new module
    modules[modules_size].alias = strdup(alias);
    if (modules[modules_size].alias) {
        modules[modules_size].module_ast = ast;
        modules_size++;
        
        // Register all functions and constants from the module with module prefix
        if (ast) {
            for (int i = 0; i < ast->child_count; i++) {
                ASTNode* n = &ast->children[i];
                if (n->type == AST_FUNC && n->text) {
                    // Register function with module prefix for namespaced access
                    char prefixed_name[256];
                    snprintf(prefixed_name, sizeof(prefixed_name), "%s.%s", alias, n->text);
                    register_function(prefixed_name, n);
                    
                    // Also register without prefix for backward compatibility
                    register_function(n->text, n);
                } else if (n->type == AST_LET && n->child_count >= 2) {
                    // Register constants (variables) with module prefix
                    const char* const_name = n->children[0].text;
                    if (const_name) {
                        // For constants, we need to evaluate them and store with module prefix
                        // This is a simplified approach - in a full implementation you'd want
                        // to store the AST and evaluate on demand
                        char prefixed_name[256];
                        snprintf(prefixed_name, sizeof(prefixed_name), "%s.%s", alias, const_name);
                        
                        // Evaluate the constant value and store it
                        if (n->children[1].type == AST_EXPR && n->children[1].text) {
                            if (is_string_literal(n->children[1].text)) {
                                // String constant - extract the value between quotes
                                size_t len = strlen(n->children[1].text);
                                if (len >= 2) {
                                    char* value = (char*)malloc(len - 1);
                                    if (value) {
                                        strncpy(value, n->children[1].text + 1, len - 2);
                                        value[len - 2] = '\0';
                                        set_str_value(prefixed_name, value);
                                        // String constant registered successfully
                                        free(value);
                                    }
                                }
                            } else {
                                // Numeric constant - evaluate it
                                long long value = eval_expression(&n->children[1]);
                                set_var_value(prefixed_name, value);
                                // Numeric constant registered successfully
                            }
                        }
                    }
                }
            }
        }
    }
}

static ASTNode* find_function_in_module(ASTNode* mod, const char* name) {
    if (!mod) return NULL;
    for (int i = 0; i < mod->child_count; i++) {
        ASTNode* n = &mod->children[i];
        if (n->type == AST_FUNC && n->text && strcmp(n->text, name) == 0) return n;
    }
    return NULL;
}

static ASTNode* resolve_module(const char* alias) {
    for (int i = 0; i < modules_size; i++) {
        if (strcmp(modules[i].alias, alias) == 0) return modules[i].module_ast;
    }
    return NULL;
}

// Find a function in a specific module by module name and function name
static ASTNode* find_function_with_module_prefix(const char* module_name, const char* function_name) {
    ASTNode* mod = resolve_module(module_name);
    if (mod) {
        return find_function_in_module(mod, function_name);
    }
    return NULL;
}

// Find a function in any loaded module (for backward compatibility)
static ASTNode* find_function_in_any_module(const char* name) {
    for (int i = 0; i < modules_size; i++) {
        ASTNode* fn = find_function_in_module(modules[i].module_ast, name);
        if (fn) return fn;
    }
    return NULL;
}

// Interpret a user-defined function call: evaluate args, bind params, execute body, capture return
static long long eval_user_function_call(ASTNode* fn, ASTNode* args_node) {
    if (!fn) return 0;
    // find body index
    int body_index = -1;
    for (int i = 0; i < fn->child_count; i++) {
        if (fn->children[i].type == AST_BLOCK) { body_index = i; break; }
    }
    if (body_index < 0) return 0;
    // collect parameter names (AST_EXPR before body, excluding type markers like 'int' and 'string')
    int param_indices[16]; int param_count = 0;
    for (int i = 0; i < body_index && param_count < 16; i++) {
        if (fn->children[i].type == AST_EXPR && fn->children[i].text && 
            strcmp(fn->children[i].text, "int") != 0 && 
            strcmp(fn->children[i].text, "string") != 0) {
            param_indices[param_count++] = i;
        }
    }
    // evaluate arguments
    long long argvals[16]; int argn = args_node ? args_node->child_count : 0;
    for (int i = 0; i < argn && i < 16; i++) {
        argvals[i] = eval_expression(&args_node->children[i]);
        if (error_occurred) return 0;
    }
    // new scope snapshot
    int old_env = var_env_size;
    // bind params
    for (int i = 0; i < param_count && i < argn; i++) {
        const char* pname = fn->children[param_indices[i]].text;
        
        // Bind parameter to both environments for maximum compatibility
        if (argvals[i] == 1) {
            // String result indicator, bind as string parameter
            const char* str_result = NULL;
            
            // Try to get the string value from the argument
            if (args_node->children[i].text) {
                // First try to get it as a string variable
                str_result = get_str_value(args_node->children[i].text);
                
                // If not found, try to get it from the temporary variable created during evaluation
                if (!str_result) {
                    char temp_var_name[64];
                    snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_var_%s", args_node->children[i].text);
                    str_result = get_str_value(temp_var_name);
                }
                
                // NEW: Also try to get it from string literal temporary variables
                if (!str_result) {
                    char temp_lit_name[64];
                    snprintf(temp_lit_name, sizeof(temp_lit_name), "__temp_str_lit_%p", (void*)&args_node->children[i]);
                    str_result = get_str_value(temp_lit_name);
                }
            }
            
            if (str_result) {
                set_str_value(pname, str_result);
                // Also bind as numeric for compatibility (convert string to number if possible)
                char* end;
                long long num_val = strtoll(str_result, &end, 10);
                if (*end == '\0') {
                    set_var_value(pname, num_val);
                } else {
                    set_var_value(pname, 0);
                }
            } else {
                set_str_value(pname, "");
                set_var_value(pname, 0);
            }
        } else {
            // Numeric result, bind as numeric parameter
            set_var_value(pname, argvals[i]);
            // Also bind as string for compatibility
            char temp_str[64];
            snprintf(temp_str, sizeof(temp_str), "%lld", argvals[i]);
            set_str_value(pname, temp_str);
        }
    }
    // execute

    

    
    int saved_return_flag = return_flag; long long saved_return_value = return_value;
    return_flag = 0; return_value = 0;
    
    eval_evaluate(&fn->children[body_index]);
    
    long long rv = return_value;
    // restore return state
    return_flag = saved_return_flag; return_value = saved_return_value;
    // restore scope
    for (int i = var_env_size - 1; i >= old_env; i--) {
        free(var_env[i].name);
    }
    var_env_size = old_env;
    return rv;
}

long long eval_expression(ASTNode* ast) {
    if (!ast) return 0;
    if (error_occurred) return 0;

    // Update current line for expression-level errors
    if (ast->line > 0) {
        current_line = ast->line;
    }
    
    // Handle string literals first
    if (ast->text && is_string_literal(ast->text)) {
        // Extract the string value (remove quotes)
        size_t len = strlen(ast->text);
        if (len >= 2) {
            char* value = (char*)malloc(len - 1);
            if (value) {
                strncpy(value, ast->text + 1, len - 2);
                value[len - 2] = '\0';
                
                // Store in a temporary variable for parameter binding
                char temp_var_name[64];
                snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_lit_%p", (void*)ast);
                set_str_value(temp_var_name, value);
                free(value);
            }
        }
        return 1; // Return 1 to indicate this is a string
    }
    
    // Handle dot expressions (method calls) first
    if (ast->type == AST_DOT) {
        if (ast->child_count >= 2) {
            // Left side is the object (string variable name)
            ASTNode* obj_node = &ast->children[0];
            const char* obj_name = NULL;
            
            if (obj_node->type == AST_EXPR && obj_node->text) {
                obj_name = obj_node->text;
            }
            
            if (!obj_name) return 0;
            
            // Right side is the method name
            ASTNode* method_node = &ast->children[1];
            const char* method_name = NULL;
            
            if (method_node->type == AST_EXPR && method_node->text) {
                method_name = method_node->text;
            }
            
            if (!method_name) return 0;
            
            // Get the string value
            const char* str_val = get_str_value(obj_name);
            if (!str_val) str_val = "";
            
            // Handle different string methods
            if (strcmp(method_name, "join") == 0) {
                // .join(separator) - returns the string itself (no-op for single strings)
                return 0; // Just return success
            } else if (strcmp(method_name, "split") == 0) {
                // .split(separator) - for now just return success
                // TODO: Implement actual splitting logic
                return 0;
            } else if (strcmp(method_name, "length") == 0) {
                // .length - return string length
                return (int)strlen(str_val);
            } else if (strcmp(method_name, "upper") == 0) {
                // .upper() - convert to uppercase
                if (str_val && *str_val) {
                    char* upper = strdup(str_val);
                    if (upper) {
                        for (int i = 0; upper[i]; i++) {
                            upper[i] = toupper(upper[i]);
                        }
                        set_str_value(obj_name, upper);
                        free(upper);
                    }
                }
                return 0;
            } else if (strcmp(method_name, "lower") == 0) {
                // .lower() - convert to lowercase
                if (str_val && *str_val) {
                    char* lower = strdup(str_val);
                    if (lower) {
                        for (int i = 0; lower[i]; i++) {
                            lower[i] = tolower(lower[i]);
                        }
                        set_str_value(obj_name, lower);
                        free(lower);
                    }
                }
                return 0;
            } else if (strcmp(method_name, "trim") == 0) {
                // .trim() - remove leading/trailing whitespace
                if (str_val && *str_val) {
                    char* trimmed = strdup(str_val);
                    if (trimmed) {
                        char* start = trimmed;
                        char* end = trimmed + strlen(trimmed) - 1;
                        
                        // Skip leading whitespace
                        while (*start && isspace(*start)) start++;
                        
                        // Skip trailing whitespace
                        while (end > start && isspace(*end)) end--;
                        
                        *(end + 1) = '\0';
                        
                        if (start > trimmed) {
                            memmove(trimmed, start, strlen(start) + 1);
                        }
                        
                        set_str_value(obj_name, trimmed);
                        free(trimmed);
                    }
                }
                return 0;
            }
        }
        return 0;
    }

    if (ast->type == AST_EXPR && ast->text) {
        if (strcmp(ast->text, "+") == 0 || strcmp(ast->text, "-") == 0 ||
            strcmp(ast->text, "*") == 0 || strcmp(ast->text, "/") == 0 ||
            strcmp(ast->text, "%") == 0 || strcmp(ast->text, "==") == 0 ||
            strcmp(ast->text, "!=") == 0 || strcmp(ast->text, "<") == 0 ||
            strcmp(ast->text, ">") == 0 || strcmp(ast->text, "<=") == 0 ||
            strcmp(ast->text, ">=") == 0) {
            
            // Check if this is string concatenation first
            if (strcmp(ast->text, "+") == 0) {
                // Check if either operand is a string
                ASTNode* left_node = &ast->children[0];
                ASTNode* right_node = &ast->children[1];
                
                const char* left_str = NULL;
                const char* right_str = NULL;
                int left_str_allocated = 0;  // Track if we allocated left_str
                int right_str_allocated = 0; // Track if we allocated right_str
                

                
                // Check if left side is a string
                if (is_string_node(left_node)) {
                    if (left_node->type == AST_EXPR && left_node->text) {
                        if (is_string_literal(left_node->text)) {
                            // It's a string literal
                            size_t len = strlen(left_node->text);
                            if (len >= 2) {
                                char* temp = malloc(len - 1);
                                if (temp) {
                                    strncpy(temp, left_node->text + 1, len - 2);
                                    temp[len - 2] = '\0';
                                    left_str = temp;
                                    left_str_allocated = 1; // Mark as allocated
                                }
                            }
                        } else if (strcmp(left_node->text, "+") == 0) {
                            // For nested + operations, we need to evaluate and get the result
                            // Since we're in string concatenation mode, we'll create a temporary variable
                            char temp_var_name[32];
                            snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_%p", (void*)left_node);
                            
                            // Evaluate the nested expression and store result
                            long long temp_result = eval_expression(left_node);
                            if (!error_occurred) {
                                if (temp_result == 1) {
                                    // String result from concatenation
                                    const char* str_result = get_str_value(temp_var_name);
                                    if (str_result) {
                                        left_str = str_result;
                                        left_str_allocated = 0; // Not allocated by us
                                    }
                                } else {
                                    // Convert numeric result to string for concatenation
                                    char temp_str[64];
                                    snprintf(temp_str, sizeof(temp_str), "%lld", temp_result);
                                    set_str_value(temp_var_name, temp_str);
                                    left_str = get_str_value(temp_var_name);
                                    left_str_allocated = 0; // Not allocated by us
                                }
                            }
                        } else {
                            // Check if it's a string variable
                            left_str = get_str_value(left_node->text);
                            left_str_allocated = 0; // Not allocated by us
                        }
                    }
                }
                
                // Check if right side is a string
                if (is_string_node(right_node)) {
                    if (right_node->type == AST_EXPR && right_node->text) {
                        if (is_string_literal(right_node->text)) {
                            // It's a string literal
                            size_t len = strlen(right_node->text);
                            if (len >= 2) {
                                char* temp = malloc(len - 1);
                                if (temp) {
                                    strncpy(temp, right_node->text + 1, len - 2);
                                    temp[len - 2] = '\0';
                                    right_str = temp;
                                    right_str_allocated = 1; // Mark as allocated
                                }
                            }
                        } else if (strcmp(right_node->text, "+") == 0) {
                            // For nested + operations, we need to evaluate and get the result
                            // Since we're in string concatenation mode, we'll create a temporary variable
                            char temp_var_name[32];
                            snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_%p", (void*)right_node);
                            
                            // Evaluate the nested expression and store result
                            long long temp_result = eval_expression(right_node);
                            if (!error_occurred) {
                                if (temp_result == 1) {
                                    // String result from concatenation
                                    const char* str_result = get_str_value(temp_var_name);
                                    if (str_result) {
                                        right_str = str_result;
                                        right_str_allocated = 0; // Not allocated by us
                                    }
                                } else {
                                    // Convert numeric result to string for concatenation
                                    char temp_str[64];
                                    snprintf(temp_str, sizeof(temp_str), "%lld", temp_result);
                                    set_str_value(temp_var_name, temp_str);
                                    right_str = get_str_value(temp_var_name);
                                    right_str_allocated = 0; // Not allocated by us
                                }
                            }
                        } else {
                            // Check if it's a string variable
                            right_str = get_str_value(right_node->text);
                            right_str_allocated = 0; // Not allocated by us
                        }
                    }
                }
                
                // If either is a string, do string concatenation
                if (left_str || right_str) {
                    const char* left_val = left_str ? left_str : "";
                    const char* right_val = right_str ? right_str : "";
                    
                    // Concatenate the strings
                    size_t left_len = strlen(left_val);
                    size_t right_len = strlen(right_val);
                    char* result = malloc(left_len + right_len + 1);
                    if (result) {
                        strcpy(result, left_val);
                        strcpy(result + left_len, right_val);
                        
                        // Clean up temporary strings - only free if they were allocated
                        if (left_str && left_str_allocated) {
                            free((char*)left_str);
                            left_str = NULL; // Prevent double-free
                        }
                        if (right_str && right_str_allocated) {
                            free((char*)right_str);
                            right_str = NULL; // Prevent double-free
                        }
                        
                        // Store the result in a temporary string variable and return a special value
                        char temp_var_name[32];
                        snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_%p", (void*)ast);
                        set_str_value(temp_var_name, result);
                        free(result);
                        return 1; // Special return value indicating string result
                    }
                    
                    // If result allocation failed, clean up temporary strings
                    if (left_str && left_str_allocated) {
                        free((char*)left_str);
                        left_str = NULL; // Prevent double-free
                    }
                    if (right_str && right_str_allocated) {
                        free((char*)right_str);
                        right_str = NULL; // Prevent double-free
                    }
                    
                    return 0;
                }
            }
            // If not string concatenation, fall through to numeric operations
            {
                // Handle numeric operations
                long long left = eval_expression(&ast->children[0]);
                if (error_occurred) return 0;
                long long right = eval_expression(&ast->children[1]);
                if (error_occurred) return 0;
                

                
                if (strcmp(ast->text, "+") == 0) return left + right;
                if (strcmp(ast->text, "-") == 0) return left - right;
                if (strcmp(ast->text, "*") == 0) return left * right;
                if (strcmp(ast->text, "/") == 0) {
                    if (right == 0) { set_error(ERROR_DIVISION_BY_ZERO); return 0; }
                    return left / right;
                }
                if (strcmp(ast->text, "%") == 0) {
                    if (right == 0) { set_error(ERROR_MODULO_BY_ZERO); return 0; }
                    return left % right;
                }
                if (strcmp(ast->text, "==") == 0) return left == right;
                if (strcmp(ast->text, "!=") == 0) return left != right;
                if (strcmp(ast->text, "<") == 0) return left < right;
                if (strcmp(ast->text, ">") == 0) return left > right;
                if (strcmp(ast->text, "<=") == 0) return left <= right;
                if (strcmp(ast->text, ">=") == 0) return left >= right;
            }
        } else if (ast->type == AST_DOT) {
            // Handle dot expressions (module.function or module.constant)
            if (ast->child_count == 2) {
                const char* module_name = ast->children[0].text;
                const char* member_name = ast->children[1].text;
                
                // Check if it's a constant first
                char prefixed_name[256];
                snprintf(prefixed_name, sizeof(prefixed_name), "%s.%s", module_name, member_name);
                
                // Check if it's a string constant
                const char* str_val = get_str_value(prefixed_name);
                if (str_val) {
                    // For string constants, we need to make the value accessible to the caller
                    // Store it in a special temporary variable that can be accessed
                    char temp_var_name[64];
                    snprintf(temp_var_name, sizeof(temp_var_name), "__dot_str_%s", prefixed_name);
                    set_str_value(temp_var_name, str_val);
                    // Return a special value to indicate this is a string
                    return 1;
                }
                
                // Check if it's a numeric constant
                if (var_exists(prefixed_name)) {
                    long long value = get_var_value(prefixed_name);
                    return value;
                }
                
                // If not a constant, it might be a function (handled in function calls)
                return 0;
            }
            return 0;
        } else if (strcmp(ast->text, "call") == 0) {
            if (ast->child_count >= 2) {
                const char* name = ast->children[0].text; // function identifier
                
                // Check if this is a method call on a string or module function call
                if (ast->children[0].type == AST_DOT) {
                    // This could be a module function call like module.function(args)
                    if (ast->children[0].child_count == 2) {
                        const char* module_name = ast->children[0].children[0].text;
                        const char* function_name = ast->children[0].children[1].text;
                        
                        // Look for the function in the specified module
                        ASTNode* fn = find_function_with_module_prefix(module_name, function_name);
                        if (fn) {
                            return eval_user_function_call(fn, &ast->children[1]);
                        } else {
                            // Function not found in module
                            fprintf(stderr, "Error: Function '%s' not found in module '%s' at line %d\n", 
                                   function_name, module_name, ast->line);
                            set_error(ERROR_UNDEFINED_VAR);
                            return 0;
                        }
                    }
                    return 0;
                }
                
                // Built-in input(prompt): prompt user and map to an action code
                if (name && strcmp(name, "input") == 0) {
                    // Choose input/output stream
                    FILE* inp = stdin;
                    FILE* outp = stdout;
#ifndef _WIN32
                    FILE* opened_tty = NULL;
                    // Prefer interactive stdin; else try a TTY device on POSIX
                    if (!isatty(STDIN_FILENO)) {
                        const char* tty_env = getenv("MYCO_TTY");
                        if (tty_env && tty_env[0]) opened_tty = fopen(tty_env, "r+");
#ifdef L_ctermid
                        if (!opened_tty) { char tty_path[L_ctermid]; ctermid(tty_path); if (tty_path[0]) opened_tty = fopen(tty_path, "r+"); }
#endif
                        if (!opened_tty) opened_tty = fopen("/dev/tty", "r+");
                        if (opened_tty) { inp = opened_tty; outp = opened_tty; }
                    }
#endif

                    // Print prompt if provided
                    if (ast->children[1].child_count > 0) {
                        ASTNode* a0 = &ast->children[1].children[0];
                        if (a0->type == AST_EXPR && a0->text && is_string_literal(a0->text)) {
                            size_t len = strlen(a0->text);
                            if (len >= 2) {
                                fwrite(a0->text + 1, 1, len - 2, outp);
                                fflush(outp);
                            }
                        }
                    }

                    char buf[256];
                    if (!fgets(buf, sizeof(buf), inp)) {
#ifndef _WIN32
                        if (inp != stdin) fclose(inp);
#endif
                        error_occurred = 1;
                        error_value = ERROR_INPUT_FAILED;
                        return ERROR_INPUT_FAILED;
                    }
#ifndef _WIN32
                    if (inp != stdin) fclose(inp);
#endif

                    // Trim whitespace
                    size_t bl = strlen(buf);
                    while (bl && (buf[bl-1] == '\n' || buf[bl-1] == '\r' || buf[bl-1] == ' ' || buf[bl-1] == '\t')) { buf[--bl] = '\0'; }
                    size_t start = 0; while (start < bl && (buf[start] == ' ' || buf[start] == '\t')) start++;
                    if (start > 0) memmove(buf, buf + start, bl - start + 1);
                    bl = strlen(buf);
                    if (bl == 0) {
                        error_occurred = 1;
                        error_value = ERROR_INVALID_INPUT;
                        return ERROR_INVALID_INPUT;
                    }

                    // If numeric, return it
                    int all_digits = 1;
                    for (size_t i = 0; i < bl; i++) if (buf[i] < '0' || buf[i] > '9') { all_digits = 0; break; }
                    if (all_digits) {
                        long long val = strtoll(buf, NULL, 10);
                        return (int)val;
                    }
                    // Map commands
                    for (size_t i = 0; i < bl; i++) buf[i] = (char)tolower((unsigned char)buf[i]);
                    if (strcmp(buf, "left") == 0) return 1;
                    if (strcmp(buf, "right") == 0) return 2;
                    if (strcmp(buf, "up") == 0) return 3;
                    if (strcmp(buf, "down") == 0) return 4;
                    if (strcmp(buf, "attack") == 0) return 5;
                    error_occurred = 1;
                    error_value = ERROR_INVALID_INPUT;
                    return ERROR_INVALID_INPUT;
                }

                // Built-in sleep(seconds)
                if (name && strcmp(name, "sleep") == 0) {
                    long long secs = 0;
                    if (ast->children[1].child_count > 0) {
                        secs = eval_expression(&ast->children[1].children[0]);
                        if (error_occurred) return 0;
                    }
#ifdef _WIN32
                    if (secs < 0) secs = 0; Sleep((DWORD)(secs * 1000));
#else
                    if (secs < 0) secs = 0; sleep((unsigned int)secs);
#endif
                    return 0;
                }

                // Built-in to_number(string)
                if (name && strcmp(name, "to_number") == 0) {
                    const char* s = NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* a0 = &ast->children[1].children[0];
                        if (a0->type == AST_EXPR && a0->text && is_string_literal(a0->text)) {
                            size_t len = strlen(a0->text); static char buf[1024]; size_t copy = len-2; if (copy > sizeof(buf)-1) copy = sizeof(buf)-1; memcpy(buf, a0->text+1, copy); buf[copy]='\0'; s = buf;
                        } else if (a0->type == AST_EXPR && a0->text) {
                            s = get_str_value(a0->text);
                        }
                    }
                    if (!s) return 0;
                    long long v = strtoll(s, NULL, 10);
                    return (int)v;
                }

                // Built-in startsWith(string, prefix)
                if (name && strcmp(name, "startsWith") == 0) {
                    const char* s = NULL; const char* pfx = NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* a0 = &ast->children[1].children[0];
                        if (a0->type == AST_EXPR && a0->text && is_string_literal(a0->text)) { size_t len=strlen(a0->text); static char b0[1024]; size_t c=len-2; if(c>sizeof(b0)-1)c=sizeof(b0)-1; memcpy(b0,a0->text+1,c); b0[c]='\0'; s=b0; } else if (a0->type==AST_EXPR && a0->text) { s=get_str_value(a0->text);} }
                    if (ast->children[1].child_count > 1) {
                        ASTNode* a1 = &ast->children[1].children[1];
                        if (a1->type == AST_EXPR && a1->text && is_string_literal(a1->text)) { size_t len=strlen(a1->text); static char b1[512]; size_t c=len-2; if(c>sizeof(b1)-1)c=sizeof(b1)-1; memcpy(b1,a1->text+1,c); b1[c]='\0'; pfx=b1; } else if (a1->type==AST_EXPR && a1->text) { pfx=get_str_value(a1->text);} }
                    if (!s || !pfx) return 0;
                    size_t lp = strlen(pfx); return strncmp(s,pfx,lp)==0 ? 1 : 0;
                }

                // Built-in json_get(body, key) -> sets json_last_value
                if (name && strcmp(name, "json_get") == 0) {
                    const char* body = NULL; const char* key = NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* a0 = &ast->children[1].children[0];
                        if (a0->type==AST_EXPR && a0->text && is_string_literal(a0->text)) { size_t len=strlen(a0->text); static char b0[4096]; size_t c=len-2; if(c>sizeof(b0)-1)c=sizeof(b0)-1; memcpy(b0,a0->text+1,c); b0[c]='\0'; body=b0; } else if (a0->type==AST_EXPR && a0->text) { body=get_str_value(a0->text);} }
                    if (ast->children[1].child_count > 1) {
                        ASTNode* a1 = &ast->children[1].children[1];
                        if (a1->type==AST_EXPR && a1->text && is_string_literal(a1->text)) { size_t len=strlen(a1->text); static char b1[256]; size_t c=len-2; if(c>sizeof(b1)-1)c=sizeof(b1)-1; memcpy(b1,a1->text+1,c); b1[c]='\0'; key=b1; } else if (a1->type==AST_EXPR && a1->text) { key=get_str_value(a1->text);} }
                    if (!body || !key) return 0;
                    // naive extraction: find "key": then read quoted string value
                    char pattern[256]; snprintf(pattern,sizeof(pattern),"\"%s\"", key);
                    const char* pos = strstr(body, pattern); if (!pos) { set_str_value("json_last_value"," "); return 0; }
                    pos = strchr(pos, ':'); if (!pos) { set_str_value("json_last_value"," "); return 0; }
                    pos++; while (*pos==' '||*pos=='\t') pos++;
                    if (*pos=='\"') { pos++; const char* start=pos; while (*pos && *pos!='\"') pos++; size_t len = (size_t)(pos-start); char* out=(char*)malloc(len+1); if (out){ memcpy(out,start,len); out[len]='\0'; set_str_value("json_last_value", out); free(out);} }
                    else { const char* start=pos; while (*pos && *pos!=',' && *pos!='\n' && *pos!='\r' && *pos!='}') pos++; size_t len=(size_t)(pos-start); char* out=(char*)malloc(len+1); if(out){ memcpy(out,start,len); out[len]='\0'; set_str_value("json_last_value", out); free(out);} }
                    return 0;
                }

                // Built-in set(varName, value): create/update a string variable
                if (name && strcmp(name, "set") == 0) {
                    const char* varName = NULL; const char* val = NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* a0 = &ast->children[1].children[0];
                        if (a0->type==AST_EXPR && a0->text && is_string_literal(a0->text)) { size_t len=strlen(a0->text); static char b0[256]; size_t c=len-2; if(c>sizeof(b0)-1)c=sizeof(b0)-1; memcpy(b0,a0->text+1,c); b0[c]='\0'; varName=b0; } else if (a0->type==AST_EXPR && a0->text) { varName=a0->text; }
                    }
                    if (ast->children[1].child_count > 1) {
                        ASTNode* a1 = &ast->children[1].children[1];
                        if (a1->type==AST_EXPR && a1->text && is_string_literal(a1->text)) { size_t len=strlen(a1->text); static char b1[4096]; size_t c=len-2; if(c>sizeof(b1)-1)c=sizeof(b1)-1; memcpy(b1,a1->text+1,c); b1[c]='\0'; val=b1; } else if (a1->type==AST_EXPR && a1->text) { const char* v=get_str_value(a1->text); if (v) val=v; }
                    }
                    if (varName && val) { set_str_value(varName, val); }
                        return 0;
                }

                // Built-in join(destName, a, b): dest = a + b
                if (name && strcmp(name, "join") == 0) {
                    const char* dest=NULL; const char* a=NULL; const char* b=NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* x = &ast->children[1].children[0];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[256]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; dest=buf; } else if (x->type==AST_EXPR && x->text) { dest=x->text; }
                    }
                    if (ast->children[1].child_count > 1) {
                        ASTNode* x = &ast->children[1].children[1];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[4096]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; a=buf; } else if (x->type==AST_EXPR && x->text) { a=get_str_value(x->text); }
                    }
                    if (ast->children[1].child_count > 2) {
                        ASTNode* x = &ast->children[1].children[2];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[4096]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; b=buf; } else if (x->type==AST_EXPR && x->text) { b=get_str_value(x->text); }
                    }
                    if (dest) {
                        const char* sa = a ? a : ""; const char* sb = b ? b : "";
                        size_t la=strlen(sa), lb=strlen(sb); char* out=(char*)malloc(la+lb+1); if (out){ memcpy(out, sa, la); memcpy(out+la, sb, lb); out[la+lb]='\0'; set_str_value(dest, out); free(out);} }
                        return 0;
                }

                // Built-in split(destArray, source, separator): split string into array
                if (name && strcmp(name, "split") == 0) {
                    const char* dest=NULL; const char* source=NULL; const char* separator=NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* x = &ast->children[1].children[0];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[256]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; dest=buf; } else if (x->type==AST_EXPR && x->text) { dest=x->text; }
                    }
                    if (ast->children[1].child_count > 1) {
                        ASTNode* x = &ast->children[1].children[1];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[4096]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; source=buf; } else if (x->type==AST_EXPR && x->text) { source=get_str_value(x->text); }
                    }
                    if (ast->children[1].child_count > 2) {
                        ASTNode* x = &ast->children[1].children[2];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[256]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; separator=buf; } else if (x->type==AST_EXPR && x->text) { separator=get_str_value(x->text); }
                    }
                    if (dest && source && separator) {
                        // For now, just store the first part in the destination
                        // TODO: Implement proper array support
                        char* pos = strstr(source, separator);
                        if (pos) {
                            size_t len = pos - source;
                            char* part = malloc(len + 1);
                            if (part) {
                                strncpy(part, source, len);
                                part[len] = '\0';
                                set_str_value(dest, part);
                                free(part);
                            }
                        } else {
                            set_str_value(dest, source);
                        }
                    }
                    return 0;
                }

                // Built-in replace(dest, source, old, new): replace occurrences
                if (name && strcmp(name, "replace") == 0) {
                    const char* dest=NULL; const char* source=NULL; const char* old=NULL; const char* new=NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* x = &ast->children[1].children[0];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[256]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; dest=buf; } else if (x->type==AST_EXPR && x->text) { dest=x->text; }
                    }
                    if (ast->children[1].child_count > 1) {
                        ASTNode* x = &ast->children[1].children[1];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[4096]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; source=buf; } else if (x->type==AST_EXPR && x->text) { source=get_str_value(x->text); }
                    }
                    if (ast->children[1].child_count > 2) {
                        ASTNode* x = &ast->children[1].children[2];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[256]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; old=buf; } else if (x->type==AST_EXPR && x->text) { old=get_str_value(x->text); }
                    }
                    if (ast->children[1].child_count > 3) {
                        ASTNode* x = &ast->children[1].children[3];
                        if (x->type==AST_EXPR && x->text && is_string_literal(x->text)) { size_t len=strlen(x->text); static char buf[256]; size_t c=len-2; if(c>sizeof(buf)-1)c=sizeof(buf)-1; memcpy(buf,x->text+1,c); buf[c]='\0'; new=buf; } else if (x->type==AST_EXPR && x->text) { new=get_str_value(x->text); }
                    }
                    if (dest && source && old && new) {
                        char* result = strdup(source);
                        char* pos = strstr(result, old);
                        if (pos) {
                            size_t old_len = strlen(old);
                            size_t new_len = strlen(new);
                            size_t result_len = strlen(result);
                            size_t new_size = result_len - old_len + new_len + 1;
                            char* new_result = realloc(result, new_size);
                            if (new_result) {
                                result = new_result;
                                memmove(pos + new_len, pos + old_len, strlen(pos + old_len) + 1);
                                memcpy(pos, new, new_len);
                                set_str_value(dest, result);
                            }
                            free(result);
                        } else {
                            set_str_value(dest, source);
                        }
                    }
                    return 0;
                }

                // Discord Gateway built-ins (POSIX only for now)
                if (name && strcmp(name, "gateway_start") == 0) {
#ifndef _WIN32
                    const char* token = NULL;
                    if (ast->children[1].child_count > 0) {
                        ASTNode* a0 = &ast->children[1].children[0];
                        if (a0->type==AST_EXPR && a0->text && is_string_literal(a0->text)) {
                            size_t len=strlen(a0->text); static char b0[256]; size_t c=len-2; if(c>sizeof(b0)-1)c=sizeof(b0)-1; memcpy(b0,a0->text+1,c); b0[c]='\0'; token=b0;
                        } else if (a0->type==AST_EXPR && a0->text) { token=get_str_value(a0->text);} }
                    if (!token) return 0;
                    if (gateway_spawn_websocat("wss://gateway.discord.gg/?v=10&encoding=json") != 0) { set_error(ERROR_FUNC_CALL); return 0; }
                    // Read hello
                    char line[8192]; if (gw_out && fgets(line, sizeof(line), gw_out)) {
                        set_str_value("gateway_last_event", line);
                        char* p = strstr(line, "heartbeat_interval"); if (p) { p = strchr(p, ':'); if (p) { p++; gw_heartbeat_ms = (int)strtol(p, NULL, 10); }}
                    }
                    // Send identify with online presence
                    if (gw_in) {
                        char id[2048];
                        snprintf(id, sizeof(id), "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":513,\"properties\":{\"os\":\"myco\",\"browser\":\"myco\",\"device\":\"myco\"},\"presence\":{\"status\":\"online\",\"activities\":[],\"afk\":false}}}\n", token);
                        fputs(id, gw_in);
                        fflush(gw_in);
                    }
                    return gw_heartbeat_ms;
#else
                    set_error(ERROR_FUNC_CALL); return 0;
#endif
                }
                if (name && strcmp(name, "gateway_pulse") == 0) {
#ifndef _WIN32
                    if (!gw_in) return 0;
                    char hb[256];
                    if (gw_seq >= 0) snprintf(hb, sizeof(hb), "{\"op\":1,\"d\":%d}\n", gw_seq);
                    else snprintf(hb, sizeof(hb), "{\"op\":1,\"d\":null}\n");
                    fputs(hb, gw_in); fflush(gw_in); return 1;
#else
                    return 0;
#endif
                }
                if (name && strcmp(name, "gateway_read") == 0) {
#ifndef _WIN32
                    if (!gw_out) return 0;
                    char line[8192]; if (!fgets(line, sizeof(line), gw_out)) return 0;
                    set_str_value("gateway_last_event", line);
                    // update sequence if present
                    char* s = strstr(line, "\"s\":"); if (s) { s+=4; gw_seq = (int)strtol(s, NULL, 10); }
                    // return op code if present
                    char* op = strstr(line, "\"op\":"); if (op) { op+=5; return (int)strtol(op, NULL, 10); }
                    return 0;
#else
                    return 0;
#endif
                }
                if (name && strcmp(name, "gateway_poll") == 0) {
#ifndef _WIN32
                    if (!gw_out) return 0;
                    fd_set rfds; FD_ZERO(&rfds); FD_SET(gw_out_fd, &rfds);
                    struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;
                    int ready = select(gw_out_fd+1, &rfds, NULL, NULL, &tv);
                    if (ready <= 0) return 0;
                    char line[8192]; if (!fgets(line, sizeof(line), gw_out)) return 0;
                    set_str_value("gateway_last_event", line);
                    char* s = strstr(line, "\"s\":"); if (s) { s+=4; gw_seq = (int)strtol(s, NULL, 10); }
                    char* op = strstr(line, "\"op\":"); if (op) { op+=5; return (int)strtol(op, NULL, 10); }
                    return 1;
#else
                    return 0;
#endif
                }

                // No built-in factorial; all function calls must resolve to user-defined or imported functions
                // Alias-wrapped call: alias(function(args)) -> resolve in module or native and interpret
                {
                    ASTNode* mod_ast = resolve_module(name);
                    const char* native_name = NULL;
                    if (!mod_ast) native_name = name;
                    if (mod_ast && ast->children[1].child_count == 1) {
                        ASTNode* inner = &ast->children[1].children[0];
                        if (inner->type == AST_EXPR && inner->text && strcmp(inner->text, "call") == 0 && inner->child_count >= 2) {
                            const char* inner_name = inner->children[0].text;
                            ASTNode* fn = find_function_in_module(mod_ast, inner_name);
                            if (fn) return eval_user_function_call(fn, &inner->children[1]);
                        }
                    } else if (native_name && ast->children[1].child_count == 1) {
                        ASTNode* inner = &ast->children[1].children[0];
                        if (inner->type == AST_EXPR && inner->text && strcmp(inner->text, "call") == 0 && inner->child_count >= 2) {
                            const char* inner_name = inner->children[0].text;
                            const char* sargs[4] = {0}; int nargs = inner->children[1].child_count; if (nargs > 4) nargs = 4;
                            for (int i = 0; i < nargs; i++) {
                                ASTNode* a = &inner->children[1].children[i]; const char* sval = NULL;
                                if (a->type == AST_EXPR && a->text && is_string_literal(a->text)) { size_t len = strlen(a->text); if (len >= 2) { static char buf[1024]; size_t copy = len - 2; if (copy > sizeof(buf)-1) copy = sizeof(buf)-1; memcpy(buf, a->text+1, copy); buf[copy] = '\0'; sval = buf; } }
                                else if (a->type == AST_EXPR && a->text) { const char* v = get_str_value(a->text); if (v) sval = v; }
                                sargs[i] = sval;
                            }
                            if (strcmp(native_name, "http") == 0 && strcmp(inner_name, "request") == 0) {
                                const char* method = sargs[0] ? sargs[0] : "GET"; const char* url = sargs[1] ? sargs[1] : ""; const char* headers = sargs[2]; const char* body = sargs[3];
                                char cmd[8192];
#ifdef _WIN32
                                const char* shq = "\"";
#else
                                const char* shq = "'";
#endif
                                const char* body_file = "myco_http_body.tmp"; const char* code_file = "myco_http_code.tmp";
                                char header_part[4096] = "";
                                if (headers && headers[0]) {
                                    const char* h = headers;
                                    while (*h) {
                                        // extract one line up to \n or \r
                                        const char* line_start = h;
                                        while (*h && *h != '\n' && *h != '\r') h++;
                                        size_t len = (size_t)(h - line_start);
                                        if (len > 0) {
                                            size_t cur = strlen(header_part);
                                            if (cur + len + 8 < sizeof(header_part)) {
                                                snprintf(header_part + cur, sizeof(header_part) - cur, " -H %s%.*s%s", shq, (int)len, line_start, shq);
                                            }
                                        }
                                        while (*h == '\n' || *h == '\r') h++;
                                    }
                                }
                                char data_part[2048] = ""; if (body && body[0]) snprintf(data_part, sizeof(data_part), " -d %s%s%s", shq, body, shq);
#ifdef _WIN32
                                snprintf(cmd, sizeof(cmd), "curl -sS -X %s%s%s %s%s%s -o %s -w \"%%{http_code}\" > %s", method, header_part, data_part, shq, url, shq, body_file, code_file);
#else
                                snprintf(cmd, sizeof(cmd), "curl -sS -X %s%s%s %s%s%s -o %s -w '%%{http_code}' > %s", method, header_part, data_part, shq, url, shq, body_file, code_file);
#endif
                                int rc = system(cmd); int http_code = 0; FILE* cf = fopen(code_file, "r"); if (cf) { fscanf(cf, "%d", &http_code); fclose(cf);} FILE* bf = fopen(body_file, "r"); if (bf) { fseek(bf,0,SEEK_END); long sz=ftell(bf); fseek(bf,0,SEEK_SET); char* buf=(char*)malloc(sz+1); if(buf){ fread(buf,1,sz,bf); buf[sz]='\0'; set_str_value("http_last_body", buf); free(buf);} fclose(bf);} remove(body_file); remove(code_file);
                                if (rc != 0 && http_code == 0) { error_occurred = 1; error_value = ERROR_INPUT_FAILED; return error_value; }
                                return http_code;
                            }
                            if (strcmp(native_name, "json") == 0 && strcmp(inner_name, "stringify") == 0) { const char* v = sargs[0] ? sargs[0] : ""; char out[1024]; snprintf(out, sizeof(out), "\"%s\"", v); set_str_value("json_last_string", out); return 0; }
                            if (strcmp(native_name, "json") == 0 && strcmp(inner_name, "parse") == 0) { const char* v = sargs[0] ? sargs[0] : ""; set_str_value("json_last_value", v); return 0; }
                        }
                    }
                }
                
                // Look up user-defined functions if not a built-in
                // First check if it's a function defined in the current file
                ASTNode* fn = find_function_global(name);
                if (fn) {
                    return eval_user_function_call(fn, &ast->children[1]);
                }
                
                // Then check if it's a function in any loaded module
                for (int mi = 0; mi < modules_size; mi++) {
                    fn = find_function_in_module(modules[mi].module_ast, name);
                    if (fn) {
                        return eval_user_function_call(fn, &ast->children[1]);
                    }
                }
                
                // If we get here, the function is undefined
                set_error(ERROR_UNDEFINED_VAR);
                return 0;
                
                // Global or module-level function call by name
                {
                    // search modules
                    for (int mi = 0; mi < modules_size; mi++) {
                        ASTNode* fn = find_function_in_module(modules[mi].module_ast, ast->children[0].text);
                        if (fn) return eval_user_function_call(fn, &ast->children[1]);
                    }
                    // search registered (including current file) functions
                    ASTNode* gfn = find_function_global(ast->children[0].text);
                    if (gfn) return eval_user_function_call(gfn, &ast->children[1]);
                }
            }
        } else {
            if (strcmp(ast->text, "i") == 0) return loop_counter;
            char* end; long long v = strtoll(ast->text, &end, 10);
            if (*end == '\0') return (long long)v;
            
                                            // Check if it's a string variable first
                const char* str_val = get_str_value(ast->text);
                if (str_val) {
                    // Store the string value in a temporary variable for parameter binding
                    char temp_var_name[64];
                    snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_var_%s", ast->text);
                    set_str_value(temp_var_name, str_val);
                    // Return a special value to indicate this is a string
                    // The caller (AST_PRINT) will handle string printing
                    return 1; // Success indicator for string variables
                }
            
            // Check if it's a numeric variable
            long long varv = get_var_value(ast->text);
            if (varv == 0 && !var_exists(ast->text)) { 
                // Check if it's a module-prefixed constant (e.g., discord.color_blue)
                // For now, we'll just return 0 since constants are handled differently
                // In a full implementation, you'd want to parse the module.name format
                set_error(ERROR_UNDEFINED_VAR); 
                return 0; 
            }
            return varv;
        }
    }

    return 0;
}

void eval_evaluate(ASTNode* ast) {
    if (!ast) return;
    
    // Update current line from AST node
    if (ast->line > 0) {
        current_line = ast->line;
    }
    




    switch (ast->type) {
        case AST_TRY: {
            // Save current variable environment size
            int old_var_env_size = var_env_size;
            
            // Set try block flag
            in_try_block = 1;
            
            // Reset error state for try block
            reset_error_state();
            
            // Evaluate try body
            for (int i = 0; i < ast->children[0].child_count; i++) {
                eval_evaluate(&ast->children[0].children[i]);
                if (error_occurred) {
                    break;
                }
            }
            
            // Reset try block flag
            in_try_block = 0;
            
            // If error occurred in try block, evaluate catch block
            if (error_occurred) {
                // Save the error that occurred
                int caught_error = error_value;
                
                // Reset error state for catch block
                reset_error_state();
                
                // Set the error variable with the caught error code
                set_var_value(ast->children[1].text, caught_error);
                
                // Mark that we are in catch for printing behavior
                in_catch_block = 1;
                // Evaluate catch body
                eval_evaluate(&ast->children[2]);
                in_catch_block = 0;
                // After catch block, reset error state since error was handled
                reset_error_state();
            }
            
            // Restore variable environment to state before try block
            for (int i = var_env_size - 1; i >= old_var_env_size; i--) {
                free(var_env[i].name);
            }
            var_env_size = old_var_env_size;
            break;
        }
        case AST_FUNC: {
            // Register function for later calls
            if (ast->text) register_function(ast->text, ast);
            break;
        }
        case AST_LET: {
            
            // Evaluate the right-hand side and assign to the variable
            if (ast->children[1].type == AST_EXPR && ast->children[1].text && is_string_literal(ast->children[1].text)) {
                size_t len = strlen(ast->children[1].text);
                char* tmp = (char*)malloc(len - 1);
                if (tmp) { memcpy(tmp, ast->children[1].text + 1, len - 2); tmp[len - 2] = '\0'; set_str_value(ast->children[0].text, tmp); free(tmp); }
            } else if (ast->children[1].type == AST_EXPR && ast->children[1].text) {
                const char* sv = get_str_value(ast->children[1].text);
                if (sv) {
                    set_str_value(ast->children[0].text, sv);
                } else {
                    long long value = eval_expression(&ast->children[1]);
                    if (!error_occurred) {
                        if (value == 1) {
                            // Special case: string result from concatenation
                            // Look for the temporary string variable
                            char temp_var_name[32];
                            snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_%p", (void*)&ast->children[1]);
                            const char* str_result = get_str_value(temp_var_name);
                            if (str_result) {

                                set_str_value(ast->children[0].text, str_result);
                                
                                // Clean up the temporary variable after use
                                // Find and remove it from the string environment
                                for (int i = 0; i < str_env_size; i++) {
                                    if (strcmp(str_env[i].name, temp_var_name) == 0) {

                                        free(str_env[i].name);
                                        free(str_env[i].value);
                                        // Move the last element to this position
                                        if (i < str_env_size - 1) {
                                            str_env[i] = str_env[str_env_size - 1];
                                        }
                                        str_env_size--;
                                        break;
                                    }
                                }
                                
                                // Clean up ALL temporary variables that were created during concatenation
                                // This is simpler and more robust than tracking cleanup markers

                                // Use a safer approach: collect indices to remove, then remove them in reverse order
                                int indices_to_remove[100]; // Should be enough for any reasonable case
                                int remove_count = 0;
                                
                                // First pass: collect indices of temporary variables to remove
                                for (int i = 0; i < str_env_size; i++) {
                                    if (strncmp(str_env[i].name, "__temp_", 7) == 0) {
                                        indices_to_remove[remove_count++] = i;
                                    }
                                }
                                
                                // Second pass: remove temporary variables in reverse order to avoid index shifting issues
                                for (int j = remove_count - 1; j >= 0; j--) {
                                    int i = indices_to_remove[j];

                                    free(str_env[i].name);
                                    free(str_env[i].value);
                                    // Move the last element to this position
                                    if (i < str_env_size - 1) {
                                        str_env[i] = str_env[str_env_size - 1];
                                    }
                                    str_env_size--;
                                }
                                                    } else {
                            set_var_value(ast->children[0].text, value);
                        }
                        } else {
                            set_var_value(ast->children[0].text, value);
                        }
                    }
                }
            } else {
                long long value = eval_expression(&ast->children[1]);
                if (!error_occurred) {
                    if (value == 1) {
                        // Special case: string result from concatenation
                        // Look for the temporary string variable
                        char temp_var_name[32];
                        snprintf(temp_var_name, sizeof(temp_var_name), "__temp_str_%p", (void*)&ast->children[1]);
                        const char* str_result = get_str_value(temp_var_name);
                        if (str_result) {
                            set_str_value(ast->children[0].text, str_result);
                            
                            // Clean up the temporary variable after use
                            // Find and remove it from the string environment
                            for (int i = 0; i < str_env_size; i++) {
                                if (strcmp(str_env[i].name, temp_var_name) == 0) {
                                    free(str_env[i].name);
                                    free(str_env[i].value);
                                    // Move the last element to this position
                                    if (i < str_env_size - 1) {
                                        str_env[i] = str_env[str_env_size - 1];
                                    }
                                    str_env_size--;
                                    break;
                                }
                            }
                            
                            // Clean up ALL temporary variables that were created during concatenation
                            // This is simpler and more robust than tracking cleanup markers
                            // Use a safer approach: collect indices to remove, then remove them in reverse order
                            int indices_to_remove2[100]; // Should be enough for any reasonable case
                            int remove_count2 = 0;
                            
                            // First pass: collect indices of temporary variables to remove
                            for (int i = 0; i < str_env_size; i++) {
                                if (strncmp(str_env[i].name, "__temp_", 7) == 0) {
                                    indices_to_remove2[remove_count2++] = i;
                                }
                            }
                            
                            // Second pass: remove temporary variables in reverse order to avoid index shifting issues
                            for (int j = remove_count2 - 1; j >= 0; j--) {
                                int i = indices_to_remove2[j];
                                free(str_env[i].name);
                                free(str_env[i].value);
                                // Move the last element to this position
                                if (i < str_env_size - 1) {
                                    str_env[i] = str_env[str_env_size - 1];
                                }
                                str_env_size--;
                            }
                        } else {
                            set_var_value(ast->children[0].text, value);
                        }
                    } else {
                        set_var_value(ast->children[0].text, value);
                    }
                }
            }
            break;
        }
        case AST_PRINT: {
            // Evaluate and print each argument
            for (int i = 0; i < ast->child_count; i++) {
                if (ast->children[i].type == AST_EXPR) {
                    // If it's a string literal, print it without quotes
                    if (is_string_literal(ast->children[i].text)) {
                        size_t len = strlen(ast->children[i].text);
                        printf("%.*s", (int)(len-2), ast->children[i].text+1);
                    } else {
                        // Otherwise evaluate the expression
                        long long value = eval_expression(&ast->children[i]);
                        
                        // If evaluation returned 1, it's a string variable
                        if (value == 1 && ast->children[i].text) {
                            const char* sv = get_str_value(ast->children[i].text);
                            if (sv) { printf("%s", sv); goto after_print_arg; }
                        }
                        
                        // If identifier matches a string var, print it
                        if (ast->children[i].text) {
                            const char* sv = get_str_value(ast->children[i].text);
                            if (sv) { printf("%s", sv); goto after_print_arg; }
                        }
                        
                        // If evaluation produced an error code, print the error message instead of 0
                        if (is_error_code((int)value)) {
                            char msg[256];
                            format_error_message((int)value, current_line, msg, sizeof(msg));
                            printf("%s", msg);
                            // reset for further prints in same line
                            reset_error_state();
                        } else if (!error_occurred) {
                            // print as 64-bit signed
                            printf("%lld", value);
                        }
after_print_arg:
                        ;
                    }
                    if (i < ast->child_count - 1 && !error_occurred) {
                        printf(" ");
                    }
                }
            }
            if (!error_occurred) {
                printf("\n");
            }
            break;
        }
        case AST_IF: {
            // Evaluate condition
            int condition = eval_expression(&ast->children[0]);
            if (!error_occurred) {  // Only proceed if no error occurred
            if (condition) {
                // Execute if body
                eval_evaluate(&ast->children[1]);
            } else if (ast->child_count > 2) {
                // Execute else body if it exists
                eval_evaluate(&ast->children[2]);
                }
            }
            break;
        }
        case AST_WHILE: {
            while (!error_occurred) {
                long long cond = eval_expression(&ast->children[0]);
                if (error_occurred) break;
                if (!cond) break;
                eval_evaluate(&ast->children[1]);
                if (return_flag || error_occurred) break;
            }
            break;
        }
        case AST_FOR: {
            // Get range start and end
            int start = eval_expression(&ast->children[1]);
            if (error_occurred) break;
            int end = eval_expression(&ast->children[2]);
            if (error_occurred) break;
            
            // Execute loop body for each value
            for (loop_counter = start; loop_counter <= end; loop_counter++) {
                eval_evaluate(&ast->children[3]);
                if (error_occurred) {
                    break;
                }
            }
            break;
        }
        case AST_SWITCH: {
            // Evaluate switch expression
            int value = eval_expression(&ast->children[0]);
            if (!error_occurred) {  // Only proceed if no error occurred
            // Find matching case
            int found_match = 0;
            for (int i = 0; i < ast->children[1].child_count; i++) {
                ASTNode* case_node = &ast->children[1].children[i];
                if (case_node->type == AST_CASE) {
                    int case_value = eval_expression(&case_node->children[0]);
                        if (!error_occurred && value == case_value) {
                        eval_evaluate(&case_node->children[1]);
                        found_match = 1;
                        break;
                    }
                    } else if (case_node->type == AST_DEFAULT && !found_match && !error_occurred) {
                    eval_evaluate(&case_node->children[0]);
                    }
                }
            }
            break;
        }
        case AST_BLOCK: {
            // handle 'use' directive blocks or execute normal blocks
            if (ast->text && strcmp(ast->text, "use") == 0 && ast->child_count == 2) {
                const char* path = ast->children[0].text;
                const char* alias = ast->children[1].text;

                // Native modules: http, json
                if ((strcmp(path, "http") == 0) || (strcmp(path, "json") == 0)) {
                    register_module(alias, NULL);
                    break;
                }

                // Compute full path and module dir
                char full[2048]; compute_full_path(path, full, sizeof(full));
                ASTNode* mod = load_and_parse_module(path);
                if (mod) {
                    register_module(alias, mod);
                    // Push base_dir to module's directory for nested imports and top-level init
                    char saved_base[1024]; strncpy(saved_base, base_dir, sizeof(saved_base)-1); saved_base[sizeof(saved_base)-1] = '\0';
                    char* last_slash = strrchr(full, '/');
                    if (last_slash) {
                        size_t n = (size_t)(last_slash - full);
                        char mod_dir[1024]; if (n >= sizeof(mod_dir)) n = sizeof(mod_dir)-1; memcpy(mod_dir, full, n); mod_dir[n] = '\0';
                        eval_set_base_dir(mod_dir);
                    }
                    // Evaluate module top-level (non-function) statements to init state
                    for (int i = 0; i < mod->child_count; i++) {
                        ASTNode* child = &mod->children[i];
                        if (child->type != AST_FUNC) {
                            eval_evaluate(child);
                            if (error_occurred && in_try_block) break;
                        }
                    }
                    // Restore base_dir
                    eval_set_base_dir(saved_base);
                }
                else {
                    char msg[256];
                    set_error(ERROR_FUNC_CALL);
                    format_error_message(error_value, ast->line, msg, sizeof(msg));
                    fprintf(stderr, "%s\n", msg);
                    reset_error_state();
                }
            } else {
                // Execute each child statement in the block
                for (int i = 0; i < ast->child_count; i++) {
                    eval_evaluate(&ast->children[i]);
                    if ((error_occurred && in_try_block) || return_flag) {
                        break;
                    }
                }
            }
            break;
        }
        case AST_RETURN: {
            long long value = 0;
            if (ast->child_count > 0) {
                value = eval_expression(&ast->children[0]);
            }
            return_value = value;
            return_flag = 1;
            break;
        }
        case AST_DOT: {
            // Handle dot expressions (module.function or module.constant)
            if (ast->child_count == 2) {
                // Left side should be the module name (identifier)
                if (ast->children[0].type == AST_EXPR && ast->children[0].text) {
                    const char* module_name = ast->children[0].text;
                    
                    // Right side should be the member name (function or constant)
                    if (ast->children[1].type == AST_EXPR && ast->children[1].text) {
                        const char* member_name = ast->children[1].text;
                        
                        // First check if it's a function in the specified module
                        ASTNode* fn = find_function_with_module_prefix(module_name, member_name);
                        if (fn) {
                            // This is a module function call, store the function for later execution
                            // We'll handle the actual call in the expression evaluator
                            break;
                        }
                        
                        // If not a function, check if it's a constant
                        char prefixed_name[256];
                        snprintf(prefixed_name, sizeof(prefixed_name), "%s.%s", module_name, member_name);
                        
                        // Check if it's a string constant
                        const char* str_val = get_str_value(prefixed_name);
                        if (str_val) {
                            // This is a string constant, we'll handle it in the expression evaluator
                            break;
                        }
                        
                        // Check if it's a numeric constant
                        if (var_exists(prefixed_name)) {
                            // This is a numeric constant, we'll handle it in the expression evaluator
                            break;
                        }
                        
                        // Neither function nor constant found
                        fprintf(stderr, "Error: Member '%s' not found in module '%s' at line %d\n", 
                               member_name, module_name, ast->line);
                        set_error(ERROR_UNDEFINED_VAR);
                        break;
                    }
                }
            }
            break;
        }
        case AST_EXPR: {
            // Handle expression statements (like function calls)
            if (ast->text && strcmp(ast->text, "expr_stmt") == 0 && ast->child_count > 0) {
                // This is an expression statement, evaluate the child expression
                eval_expression(&ast->children[0]);
            }
            break;
        }
    }

    // Evaluate children for non-control flow nodes (exclude FUNC as well)
    if (ast->type != AST_IF && ast->type != AST_FOR && 
        ast->type != AST_SWITCH && ast->type != AST_TRY &&
        ast->type != AST_FUNC && ast->type != AST_BLOCK && ast->type != AST_RETURN) {
        for (int i = 0; i < ast->child_count; i++) {
            eval_evaluate(&ast->children[i]);
            if (return_flag) break;
        }
    }

    // Evaluate next node in linked list
    // Temporarily disabled to debug double-free issue
    // if (ast->next) {
    //     eval_evaluate(ast->next);
    // }
}

// Function to clear module AST references to prevent double-free
void eval_clear_module_asts() {
    if (modules && modules_size > 0) {
        for (int i = 0; i < modules_size; i++) {
            modules[i].module_ast = NULL;
        }
    }
}

// Function to clear function AST references to prevent double-free
void eval_clear_function_asts() {
    if (functions && functions_size > 0) {
        for (int i = 0; i < functions_size; i++) {
            functions[i].func_ast = NULL;
        }
    }
}

// Function to reset all environments for fresh execution
void eval_reset_environments() {
    cleanup_all_environments();
    
    // Reset error state
    error_occurred = 0;
    error_value = 0;
    current_line = 0;
    return_flag = 0;
    return_value = 0;
    in_try_block = 0;
    in_catch_block = 0;
    
    // Reset loop counter
    loop_counter = 0;
} 