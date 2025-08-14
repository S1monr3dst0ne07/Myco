/**
 * @file eval.c
 * @brief Myco Language Evaluator - Executes Abstract Syntax Tree (AST)
 * @version 1.0.0
 * @author Myco Development Team
 * 
 * This file implements the evaluation phase of the Myco interpreter.
 * It takes the parsed AST and executes the program, handling all runtime
 * operations including variable management, function calls, and control flow.
 * 
 * Evaluator Features:
 * - AST execution with scope management
 * - Variable environment with reassignment support
 * - Function call handling with parameter binding
 * - Module system with import/export
 * - Error handling and reporting
 * - Memory management and cleanup
 * - Loop execution with safety limits
 * 
 * Core Components:
 * - Variable environment (local and global scope)
 * - Function registry and lookup
 * - Module registry and management
 * - Error state management
 * - Loop execution state management
 * 
 * Architecture:
 * - Recursive AST traversal
 * - Dynamic scope management
 * - Memory-efficient variable storage
 * - Cross-platform compatibility
 */

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
#include "eval.h"
#include "lexer.h"
#include "config.h"
#include "loop_manager.h"
#include <errno.h>
#include <time.h>

// Array data structure is now defined in eval.h
#ifdef _WIN32
// Windows doesn't have curl by default, so we'll skip websocat functionality
#else
#include <curl/curl.h>
#endif

/*******************************************************************************
 * LOOP EXECUTION STATE MANAGEMENT
 ******************************************************************************/

/**
 * @brief Global loop execution state for managing loop safety
 * 
 * This state tracks active loops and enforces safety limits to prevent
 * infinite loops and excessive resource consumption during execution.
 */
static LoopExecutionState* global_loop_state = NULL;

/**
 * @brief Initializes the global loop execution state
 * 
 * Creates and sets up the loop execution state if it doesn't exist.
 * This ensures loop safety features are available during execution.
 */
static void init_loop_execution_state(void) {
    if (!global_loop_state) {
        global_loop_state = create_loop_execution_state();
    }
}

/**
 * @brief Cleans up the global loop execution state
 * 
 * Destroys the loop execution state and frees associated memory.
 * Should be called during interpreter shutdown to prevent memory leaks.
 */
void cleanup_loop_execution_state(void) {
    if (global_loop_state) {
        destroy_loop_execution_state(global_loop_state);
        global_loop_state = NULL;
    }
}

/**
 * @brief Determines if a loop should continue executing
 * @param context The loop context containing iteration state
 * @return 1 if loop should continue, 0 if it should stop
 * 
 * This function implements loop safety logic:
 * - Checks iteration limits to prevent infinite loops
 * - Handles positive and negative step values
 * - Prevents zero-step infinite loops
 * - Ensures loop termination conditions are met
 */
static int should_continue_loop(LoopContext* context) {
    if (!context) return 0;
    
    // Check if we've exceeded the maximum iterations
    if (context->iteration_count >= context->max_iterations) {
        return 0;
    }
    
    // Determine continuation based on step direction
    if (context->step_value > 0) {
        // Positive step: continue while current <= end
        return context->current_value <= context->end_value;
    } else if (context->step_value < 0) {
        // Negative step: continue while current >= end
        return context->current_value >= context->end_value;
    } else {
        // Zero step: never continue (prevent infinite loops)
        return 0;
    }
}

/*******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/

/**
 * Forward declarations for functions defined later in this file.
 * These are organized by functionality for better code organization.
 */

// Module and function management
static struct ASTNode* find_function_in_module(struct ASTNode* mod, const char* name);
static struct ASTNode* resolve_module(const char* alias);

// Core evaluation functions
long long eval_expression(struct ASTNode* ast);
void eval_evaluate(struct ASTNode* ast);

// Function execution
static long long eval_user_function_call(struct ASTNode* fn, struct ASTNode* args_node);

// Variable and string management
static const char* get_str_value(const char* name);

// Add new function for finding functions with module prefix
static ASTNode* find_function_with_module_prefix(const char* module_name, const char* function_name);
static ASTNode* find_function_in_any_module(const char* name);

// ANSI color codes
#define RED "\033[31m"
#define RESET "\033[0m"

/*******************************************************************************
 * ERROR HANDLING SYSTEM
 ******************************************************************************/

/**
 * Myco uses a structured error code system for comprehensive error reporting.
 * 
 * Error Code Structure: 0xMY[SEV][MOD][ERR]
 * - SEV: Severity (0=Info, 1=Warning, 2=Error, F=Fatal)
 * - MOD: Module (0=Runtime, 1=Math, 2=Type, 3=Syntax, 4=IO)
 * - ERR: Specific error code within the module
 * 
 * This system allows for:
 * - Categorized error reporting
 * - Severity-based error handling
 * - Module-specific error recovery
 * - Consistent error format across the interpreter
 */

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

// Enhanced variable environment: supports numbers, strings, and arrays
typedef struct {
    char* name;
    enum {
        VAR_TYPE_NUMBER,
        VAR_TYPE_STRING,
        VAR_TYPE_ARRAY
    } type;
    long long number_value;
    char* string_value;
    MycoArray* array_value;
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
    
    // Always use base_dir for relative paths
    if (base_dir[0]) {
        snprintf(out, out_size, "%s/%s", base_dir, path_with_ext);
    } else {
        // If no base_dir, use current directory
        snprintf(out, out_size, "./%s", path_with_ext);
    }
    
    
}

// Global function registry
typedef struct {
    char* name;
    ASTNode* func_ast; // points to AST_FUNC node
} FuncEntry;
static FuncEntry* functions = NULL;
static int functions_size = 0;
static int functions_cap = 0;

// Scope stack for function calls
typedef struct {
    int var_env_start;  // Starting index in var_env for this scope
    int str_env_start;  // Starting index in str_env for this scope
} ScopeEntry;
static ScopeEntry* scope_stack = NULL;
static int scope_stack_size = 0;
static int scope_stack_capacity = 0;

// Scope management functions
static void push_scope() {
    if (scope_stack_size >= scope_stack_capacity) {
        int new_capacity = scope_stack_capacity ? scope_stack_capacity * 2 : 8;
        ScopeEntry* new_stack = (ScopeEntry*)realloc(scope_stack, new_capacity * sizeof(ScopeEntry));
        if (!new_stack) {
            fprintf(stderr, "Error: Failed to expand scope stack\n");
            return;
        }
        scope_stack = new_stack;
        scope_stack_capacity = new_capacity;
    }
    
    scope_stack[scope_stack_size].var_env_start = var_env_size;
    scope_stack[scope_stack_size].str_env_start = str_env_size;
    scope_stack_size++;
}

static void pop_scope() {
    if (scope_stack_size > 0) {
        scope_stack_size--;
        ScopeEntry* scope = &scope_stack[scope_stack_size];
        
        // Remove variables from this scope
        while (var_env_size > scope->var_env_start) {
            var_env_size--;
            if (var_env[var_env_size].name) {
                // Clean up array variables if they exist
            if (var_env[var_env_size].type == VAR_TYPE_ARRAY && var_env[var_env_size].array_value) {
                destroy_array(var_env[var_env_size].array_value);
                var_env[var_env_size].array_value = NULL;
            }
                tracked_free(var_env[var_env_size].name, __FILE__, __LINE__, "pop_scope_var");
                var_env[var_env_size].name = NULL;
            }
        }
        
        // Remove string variables from this scope
        while (str_env_size > scope->str_env_start) {
            str_env_size--;
            if (str_env[str_env_size].name) {
                tracked_free(str_env[str_env_size].name, __FILE__, __LINE__, "pop_scope_str");
                str_env[str_env_size].name = NULL;
            }
        }
    }
}

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
    // Search from the end (most recent variables first) to prioritize function parameters
    for (int i = var_env_size - 1; i >= 0; i--) {
        if (var_env[i].name && strcmp(var_env[i].name, name) == 0) {
            if (var_env[i].type == VAR_TYPE_NUMBER) {
                return var_env[i].number_value;
            } else if (var_env[i].type == VAR_TYPE_ARRAY) {
                // Return array size as a number for array variables
                return array_size(var_env[i].array_value);
            }
            // String variables return 0 (as before)
            return 0;
        }
    }
    return 0;
}

// Helper function to set a variable's value in the environment
void set_var_value(const char* name, long long value) {
    // Check if variable already exists
    for (int i = var_env_size - 1; i >= 0; i--) {
        if (strcmp(var_env[i].name, name) == 0) {
            // Update existing variable
            if (var_env[i].type == VAR_TYPE_ARRAY && var_env[i].array_value) {
                destroy_array(var_env[i].array_value);
                var_env[i].array_value = NULL;
            }
            var_env[i].type = VAR_TYPE_NUMBER;
            var_env[i].number_value = value;
            var_env[i].string_value = NULL;
            var_env[i].array_value = NULL;
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
        var_env[var_env_size].type = VAR_TYPE_NUMBER;
        var_env[var_env_size].number_value = value;
        var_env[var_env_size].string_value = NULL;
        var_env[var_env_size].array_value = NULL;
        var_env_size++;
    }
}

// Helper function to set an array variable in the environment
void set_array_value(const char* name, MycoArray* array) {
    if (!name || !array) return;
    
    // Check if variable already exists
    for (int i = var_env_size - 1; i >= 0; i--) {
        if (strcmp(var_env[i].name, name) == 0) {
            // Update existing variable
            if (var_env[i].type == VAR_TYPE_ARRAY && var_env[i].array_value) {
                destroy_array(var_env[i].array_value);
            }
            var_env[i].type = VAR_TYPE_ARRAY;
            var_env[i].array_value = array;
            var_env[i].number_value = 0;
            var_env[i].string_value = NULL;
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
        var_env[var_env_size].type = VAR_TYPE_ARRAY;
        var_env[var_env_size].array_value = array;
        var_env[var_env_size].number_value = 0;
        var_env[var_env_size].string_value = NULL;
        var_env_size++;
    }
}

// Helper function to get an array variable from the environment
MycoArray* get_array_value(const char* name) {
    // Search from the end (most recent variables first) to prioritize function parameters
    for (int i = var_env_size - 1; i >= 0; i--) {
        if (var_env[i].name && strcmp(var_env[i].name, name) == 0) {
            if (var_env[i].type == VAR_TYPE_ARRAY) {
                return var_env[i].array_value;
            }
            return NULL;
        }
    }
    return NULL;
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

/*******************************************************************************
 * ARRAY MANAGEMENT FUNCTIONS
 ******************************************************************************/

/**
 * @brief Creates a new array with specified capacity and type
 * @param initial_capacity Initial capacity for the array
 * @param is_string_array 1 for string array, 0 for number array
 * @return New array instance, or NULL on failure
 */
MycoArray* create_array(int initial_capacity, int is_string_array) {
    if (initial_capacity <= 0) initial_capacity = 8;
    
    MycoArray* array = (MycoArray*)tracked_malloc(sizeof(MycoArray), __FILE__, __LINE__, "create_array");
    if (!array) return NULL;
    
    array->capacity = initial_capacity;
    array->size = 0;
    array->is_string_array = is_string_array;
    
    if (is_string_array) {
        array->str_elements = (char**)tracked_malloc(initial_capacity * sizeof(char*), __FILE__, __LINE__, "create_array_str");
        array->elements = NULL;
        if (!array->str_elements) {
            tracked_free(array, __FILE__, __LINE__, "create_array_str_fail");
            return NULL;
        }
        // Initialize string pointers to NULL
        for (int i = 0; i < initial_capacity; i++) {
            array->str_elements[i] = NULL;
        }
    } else {
        array->elements = (long long*)tracked_malloc(initial_capacity * sizeof(long long), __FILE__, __LINE__, "create_array_num");
        array->str_elements = NULL;
        if (!array->elements) {
            tracked_free(array, __FILE__, __LINE__, "create_array_num_fail");
            return NULL;
        }
        // Initialize numbers to 0
        for (int i = 0; i < initial_capacity; i++) {
            array->elements[i] = 0;
        }
    }
    
    return array;
}

/**
 * @brief Destroys an array and frees all associated memory
 * @param array The array to destroy
 */
void destroy_array(MycoArray* array) {
    if (!array) return;
    
    if (array->is_string_array && array->str_elements) {
        // Free all string elements
        for (int i = 0; i < array->size; i++) {
            if (array->str_elements[i]) {
                tracked_free(array->str_elements[i], __FILE__, __LINE__, "destroy_array_str");
            }
        }
        tracked_free(array->str_elements, __FILE__, __LINE__, "destroy_array_str_array");
    } else if (array->elements) {
        tracked_free(array->elements, __FILE__, __LINE__, "destroy_array_num_array");
    }
    
    tracked_free(array, __FILE__, __LINE__, "destroy_array");
}

/**
 * @brief Adds an element to the end of an array
 * @param array The array to modify
 * @param element The element to add
 * @return 1 on success, 0 on failure
 */
int array_push(MycoArray* array, void* element) {
    if (!array || !element) return 0;
    
    // Expand capacity if needed
    if (array->size >= array->capacity) {
        int new_capacity = array->capacity * 2;
        if (array->is_string_array) {
            char** new_elements = (char**)tracked_realloc(array->str_elements, new_capacity * sizeof(char*), __FILE__, __LINE__, "array_push_str");
            if (!new_elements) return 0;
            array->str_elements = new_elements;
            // Initialize new elements to NULL
            for (int i = array->capacity; i < new_capacity; i++) {
                array->str_elements[i] = NULL;
            }
        } else {
            long long* new_elements = (long long*)tracked_realloc(array->elements, new_capacity * sizeof(long long), __FILE__, __LINE__, "array_push_num");
            if (!new_elements) return 0;
            array->elements = new_elements;
        }
        array->capacity = new_capacity;
    }
    
    // Add the element
    if (array->is_string_array) {
        array->str_elements[array->size] = strdup((char*)element);
    } else {
        array->elements[array->size] = *(long long*)element;
    }
    array->size++;
    
    return 1;
}

/**
 * @brief Gets an element from an array at the specified index
 * @param array The array to access
 * @param index The index of the element
 * @return Pointer to the element, or NULL if invalid
 */
void* array_get(MycoArray* array, int index) {
    if (!array || index < 0 || index >= array->size) return NULL;
    
    if (array->is_string_array) {
        return array->str_elements[index];
    } else {
        // Return pointer to the number (caller must cast appropriately)
        return &array->elements[index];
    }
}

/**
 * @brief Sets an element in an array at the specified index
 * @param array The array to modify
 * @param index The index to set
 * @param element The new element value
 * @return 1 on success, 0 on failure
 */
int array_set(MycoArray* array, int index, void* element) {
    if (!array || !element || index < 0 || index >= array->size) return 0;
    
    if (array->is_string_array) {
        // Free old string if it exists
        if (array->str_elements[index]) {
            tracked_free(array->str_elements[index], __FILE__, __LINE__, "array_set_str");
        }
        array->str_elements[index] = strdup((char*)element);
    } else {
        array->elements[index] = *(long long*)element;
    }
    
    return 1;
}

/**
 * @brief Gets the current size of an array
 * @param array The array to check
 * @return Number of elements in the array
 */
int array_size(MycoArray* array) {
    return array ? array->size : 0;
}

/**
 * @brief Gets the current capacity of an array
 * @param array The array to check
 * @return Current allocated capacity
 */
int array_capacity(MycoArray* array) {
    return array ? array->capacity : 0;
}

/**
 * @brief Cleans up all arrays in the variable environment
 */
void cleanup_array_env() {
    if (var_env && var_env_size > 0) {
        for (int i = 0; i < var_env_size; i++) {
            if (var_env[i].name && var_env[i].type == VAR_TYPE_ARRAY && var_env[i].array_value) {
                destroy_array(var_env[i].array_value);
                var_env[i].array_value = NULL;
            }
        }
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
    #if DEBUG_MEMORY_TRACKING
    printf("Cleaning up all environments...\n");
    #endif
    
    cleanup_str_env();
    cleanup_var_env();
    cleanup_array_env();
    cleanup_func_env();
    cleanup_module_env();
    
    #if DEBUG_MEMORY_TRACKING
    printf("All environments cleaned up\n");
    #endif
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
    long long argvals[16]; int argn = 0;
    
    // Find the arguments container (should be the second child)
    if (args_node && args_node->child_count >= 2) {
        ASTNode* args_container = &args_node->children[1]; // Second child should be args container
        if (args_container->text && strcmp(args_container->text, "args") == 0) {
            argn = args_container->child_count;
            for (int i = 0; i < argn && i < 16; i++) {
                argvals[i] = eval_expression(&args_container->children[i]);
                if (error_occurred) return 0;
            }
        } else {
            argn = args_node->child_count;
            for (int i = 0; i < argn && i < 16; i++) {
                argvals[i] = eval_expression(&args_node->children[i]);
                if (error_occurred) return 0;
            }
        }
    } else {
        argn = 0;
    }
    // Create new scope for function call
    push_scope();
    
    // bind params
    for (int i = 0; i < param_count && i < argn; i++) {
        const char* pname = fn->children[param_indices[i]].text;
        
        // Force parameter binding by always adding as new variable (don't update existing)
        // This ensures function parameters override global variables
        if (var_env_size >= var_env_capacity) {
            int new_capacity = var_env_capacity ? var_env_capacity * 2 : 8;
            VarEntry* new_env = (VarEntry*)realloc(var_env, new_capacity * sizeof(VarEntry));
            if (!new_env) {
                fprintf(stderr, "Error: Failed to expand variable environment\n");
                return 0;
            }
            var_env = new_env;
            var_env_capacity = new_capacity;
        }
        
        // Always add as new variable to ensure parameter binding works
        var_env[var_env_size].name = strdup(pname);
        if (var_env[var_env_size].name) {
            var_env[var_env_size].type = VAR_TYPE_NUMBER;
            var_env[var_env_size].number_value = argvals[i];
            var_env[var_env_size].array_value = NULL;
            var_env[var_env_size].string_value = NULL;
            var_env_size++;
        }
        
        // Also bind as string for compatibility
        char temp_str[64];
        snprintf(temp_str, sizeof(temp_str), "%lld", argvals[i]);
        set_str_value(pname, temp_str);
    }
    // execute

    

    
    int saved_return_flag = return_flag; long long saved_return_value = return_value;
    return_flag = 0; return_value = 0;
    
    eval_evaluate(&fn->children[body_index]);
    
    long long rv = return_value;
    // restore return state
    return_flag = saved_return_flag; return_value = saved_return_value;
    
    // Clean up function scope
    pop_scope();
    
    return rv;
}

long long eval_expression(ASTNode* ast) {
    if (!ast) {
        return 0;
    }
    
    if (error_occurred) {
        return 0;
    }

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

    // Handle numeric literals
    if (ast->text && ast->child_count == 0) {
        char* endptr;
        long long num = strtoll(ast->text, &endptr, 10);
        if (*endptr == '\0') {
            return num; // Return the numeric value
        }
    }

    // Handle array access
    if (ast->type == AST_ARRAY_ACCESS) {
        if (ast->child_count < 2) {
            fprintf(stderr, "Error: Invalid array access structure\n");
            return 0;
        }
        
        // Get the array name from the first child
        char* array_name = NULL;
        if (ast->children[0].type == AST_EXPR && ast->children[0].text) {
            array_name = ast->children[0].text;
        } else {
            fprintf(stderr, "Error: Invalid array expression at line %d\n", ast->line);
            return 0;
        }
        
        // Get the array variable
        MycoArray* array = get_array_value(array_name);
        if (!array) {
            fprintf(stderr, "Error: Array '%s' not found at line %d\n", array_name, ast->line);
            return 0;
        }
        
        // Evaluate the index expression
        long long index = eval_expression(&ast->children[1]);
        if (index < 0 || index >= array->size) {
            fprintf(stderr, "Error: Array index %lld out of bounds [0, %d] at line %d\n", 
                    index, array->size - 1, ast->line);
            return 0;
        }
        
        // Get the element value
        void* element = array_get(array, index);
        if (!element) {
            fprintf(stderr, "Error: Failed to access array element at line %d\n", ast->line);
            return 0;
        }
        
        if (array->is_string_array) {
            return 0; // String arrays return 0 for now
        } else {
            return *(long long*)element;
        }
    }
    
    // Handle function calls (text="call" with children)
    if (ast->text && strcmp(ast->text, "call") == 0 && ast->child_count >= 2) {
        // Get function name from first child
        ASTNode* func_name_node = &ast->children[0];
        
        // Check if the function name is a dot expression (module.function)
        if (func_name_node->type == AST_DOT && func_name_node->child_count >= 2) {
            // This is a module function call: module.function
            const char* module_name = func_name_node->children[0].text;
            const char* function_name = func_name_node->children[1].text;
            
            if (module_name && function_name) {
    
                
                // Find the module
                for (int i = 0; i < modules_size; i++) {
                    if (strcmp(modules[i].alias, module_name) == 0) {
                        // Found the module, now look for the function
                        ASTNode* func = find_function_in_module(modules[i].module_ast, function_name);
                        if (func) {
                            printf("DEBUG: Found function '%s' in module '%s'\n", function_name, module_name);
                            
                            // Execute the function with arguments
                            long long result = eval_user_function_call(func, ast);
                            printf("DEBUG: Module function '%s.%s' returned: %lld\n", module_name, function_name, result);
                            return result;
                        } else {
                            printf("DEBUG: Function '%s' not found in module '%s'\n", function_name, module_name);
                            return 0;
                        }
                    }
                }
                
                printf("DEBUG: Module '%s' not found\n", module_name);
                return 0;
            }
        }
        
        // Handle string-based function names (for backward compatibility)
        char* func_name = func_name_node->text;
        if (func_name) {
            
            // First, check if this is a module function call (e.g., importedModule.factorial)
            // The function name might be "importedModule.factorial" or just "factorial" if called directly
            char* dot_pos = strchr(func_name, '.');
            if (dot_pos) {
                // This is a module function call: "module.function"
                char module_name[256];
                char function_name[256];
                
                // Extract module and function names
                int module_len = dot_pos - func_name;
                strncpy(module_name, func_name, module_len);
                module_name[module_len] = '\0';
                
                strcpy(function_name, dot_pos + 1);
                

                
                // Find the module
                for (int i = 0; i < modules_size; i++) {
                    if (strcmp(modules[i].alias, module_name) == 0) {
                        // Found the module, now look for the function
                        ASTNode* func = find_function_in_module(modules[i].module_ast, function_name);
                        if (func) {
                            printf("DEBUG: Found function '%s' in module '%s'\n", function_name, module_name);
                            
                            // Execute the function with arguments
                            long long result = eval_user_function_call(func, ast);
                            printf("DEBUG: Module function '%s.%s' returned: %lld\n", module_name, function_name, result);
                            return result;
                        } else {
                            printf("DEBUG: Function '%s' not found in module '%s'\n", function_name, module_name);
                            return 0;
                        }
                    }
                }
                
                printf("DEBUG: Module '%s' not found\n", module_name);
                return 0;
            }
            
            // Regular function call - find the function globally
            ASTNode* func = find_function_global(func_name);
            if (func) {
                // Execute the function with arguments
                long long result = eval_user_function_call(func, ast);
                return result;
            } else {
                fprintf(stderr, "Error: Function '%s' not found\n", func_name);
            }
        }
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
            
            // First, check if this is a module function call
            for (int i = 0; i < modules_size; i++) {
                if (strcmp(modules[i].alias, obj_name) == 0) {
                    // Found the module, now look for the function
                ASTNode* func = find_function_in_module(modules[i].module_ast, method_name);
                if (func) {
                    // Return a special value to indicate this is a module function call
                    // The actual call will be handled by the function call logic
                    return 1; // Success indicator
                } else {
                    return 0;
                }
                }
            }
            
            // If not a module, handle as string methods
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
        
        // Check if this is a variable reference
        if (var_exists(ast->text)) {
            long long value = get_var_value(ast->text);
            return value;
        } else {
        }
        
        // Check if this is a string literal
        if (is_string_literal(ast->text)) {
            return 1; // Return 1 to indicate string
        }
        
        // Check if this is a number
        char* endptr;
        long long num = strtoll(ast->text, &endptr, 10);
        if (*endptr == '\0') {
            return num;
        }
        
        // Check if this is an operator
        if (strcmp(ast->text, "+") == 0 || strcmp(ast->text, "-") == 0 ||
            strcmp(ast->text, "*") == 0 || strcmp(ast->text, "/") == 0 ||
            strcmp(ast->text, "%") == 0 || strcmp(ast->text, "==") == 0 ||
            strcmp(ast->text, "!=") == 0 || strcmp(ast->text, "<") == 0 ||
            strcmp(ast->text, ">") == 0 || strcmp(ast->text, "<=") == 0 ||
            strcmp(ast->text, ">=") == 0 || strcmp(ast->text, "and") == 0 ||
            strcmp(ast->text, "or") == 0) {
            
                    // Handle numeric operations
        if (ast->child_count >= 2) {
            long long left = eval_expression(&ast->children[0]);
            if (error_occurred) return 0;
            long long right = eval_expression(&ast->children[1]);
            if (error_occurred) return 0;
            
            long long result = 0;
            if (strcmp(ast->text, "+") == 0) result = left + right;
            else if (strcmp(ast->text, "-") == 0) result = left - right;
            else if (strcmp(ast->text, "*") == 0) result = left * right;
            else if (strcmp(ast->text, "/") == 0) {
                if (right == 0) { set_error(ERROR_DIVISION_BY_ZERO); return 0; }
                result = left / right;
            }
            else if (strcmp(ast->text, "%") == 0) {
                if (right == 0) { set_error(ERROR_MODULO_BY_ZERO); return 0; }
                result = left % right;
            }
            else if (strcmp(ast->text, "==") == 0) result = left == right;
            else if (strcmp(ast->text, "!=") == 0) result = left != right;
            else if (strcmp(ast->text, "<") == 0) result = left < right;
            else if (strcmp(ast->text, ">") == 0) result = left > right;
            else if (strcmp(ast->text, "<=") == 0) result = left <= right;
            else if (strcmp(ast->text, ">=") == 0) result = left >= right;
            else if (strcmp(ast->text, "and") == 0) result = left && right;
            else if (strcmp(ast->text, "or") == 0) result = left || right;
            
            return result;
        }
        }
        
        return 0;
    }
    
    return 0;
}

// Main evaluation function
void eval_evaluate(ASTNode* ast) {
    if (!ast) return;

    switch (ast->type) {
        case AST_FOR: {
            // Initialize loop execution state if not already done
            if (!global_loop_state) {
                init_loop_execution_state();
            }

            // Get loop components
            if (ast->child_count < 3) {
                fprintf(stderr, "Error: Invalid for loop structure\n");
                return;
            }

            // Extract loop variable name from first child
            char* loop_var_name = ast->children[0].text;
            if (!loop_var_name) {
                fprintf(stderr, "Error: Loop variable name is NULL\n");
                return;
            }

            // Evaluate start, end, and step values
            int64_t start = eval_expression(&ast->children[1]);
            int64_t end = eval_expression(&ast->children[2]);
            
            // Check if we have a step parameter by looking at the structure
            // For loops without step: [loop_var, start, end, body] (4 children)
            // For loops with step: [loop_var, start, end, step, body] (5 children)
            int64_t step = 1; // Default step
            if (ast->child_count == 5) {
                // Loop has explicit step
                step = eval_expression(&ast->children[3]);
            }

            // Create and push loop context
            LoopContext* context = create_loop_context(loop_var_name, start, end, step, ast->line);
            if (!context) {
                fprintf(stderr, "Error: Failed to create loop context\n");
                return;
            }

            push_loop_context(global_loop_state, context);
            global_loop_state->in_loop_body = 1;

            // Execute loop
            int iterations = 0;
            while (should_continue_loop(context)) {
                // Set loop variable value
                set_var_value(loop_var_name, context->current_value);

                // Execute loop body
                if (ast->child_count == 5) {
                    // Loop with step: body is at children[4]
                    eval_evaluate(&ast->children[4]);
                } else {
                    // Loop without step: body is at children[3]
                    eval_evaluate(&ast->children[3]);
                }

                // Check for control flow
                if (global_loop_state->break_requested) {
                    global_loop_state->break_requested = 0;
                    break;
                }
                if (global_loop_state->continue_requested) {
                    global_loop_state->continue_requested = 0;
                    continue;
                }
                if (global_loop_state->return_requested) {
                    break;
                }

                // Update loop state
                context->current_value += context->step_value;
                context->iteration_count++;
                iterations++;

                // Safety check
                if (iterations > MAX_LOOP_ITERATIONS) {
                    fprintf(stderr, "Error: Maximum loop iterations exceeded at line %d\n", ast->line);
                    break;
                }
            }

            // Clean up
            global_loop_state->in_loop_body = 0;
            LoopContext* popped = pop_loop_context(global_loop_state);
            if (popped) {
                destroy_loop_context(popped);
            }

            // Update statistics
            update_loop_statistics(1, iterations, 0);

            return;
        }

        case AST_WHILE: {
            if (ast->child_count < 2) {
                fprintf(stderr, "Error: Invalid while loop structure\n");
                return;
            }

            int iterations = 0;
            while (1) {
                // Evaluate condition
                int64_t condition_result = eval_expression(&ast->children[0]);
                
                if (!condition_result) {
                    break; // Condition is false
                }

                // Execute loop body
                eval_evaluate(&ast->children[1]);

                // Check for control flow
                if (global_loop_state && global_loop_state->break_requested) {
                    global_loop_state->break_requested = 0;
                    break;
                }
                if (global_loop_state && global_loop_state->continue_requested) {
                    global_loop_state->continue_requested = 0;
                    continue;
                }
                if (global_loop_state && global_loop_state->return_requested) {
                    break;
                }

                iterations++;

                // Safety check
                if (iterations > MAX_LOOP_ITERATIONS) {
                    fprintf(stderr, "Error: Maximum while loop iterations exceeded at line %d\n", ast->line);
                    break;
                }
            }

            return;
        }

        case AST_BLOCK: {
            // Handle special block types
            if (ast->text && strcmp(ast->text, "use") == 0 && ast->child_count == 2) {
                // This is a 'use' statement: load module and create alias
                char* module_path = ast->children[0].text;
                char* alias = ast->children[1].text;
                
                // Load and parse the module
                ASTNode* module_ast = load_and_parse_module(module_path);
                if (module_ast) {
                    // Register the module with the alias
                    register_module(alias, module_ast);
                } else {
                    fprintf(stderr, "Error: Failed to load module '%s'\n", module_path);
                }
            } else {
                // Regular block - evaluate all children
                for (int i = 0; i < ast->child_count; i++) {
                    eval_evaluate(&ast->children[i]);
                    // Check if a return statement was encountered
                    if (return_flag) {
                        break;
                    }
                }
            }
            return;
        }

        case AST_PRINT: {
            if (ast->child_count == 0) {
                printf("\n");
                return;
            }

            for (int i = 0; i < ast->child_count; i++) {
                if (i > 0) printf(" ");
                
                ASTNode* arg = &ast->children[i];
                if (arg->type == AST_EXPR) {
                    if (arg->text && arg->text[0] == '"') {
                        // String literal - print without quotes
                        size_t len = strlen(arg->text);
                        if (len > 2) {
                            // Print the string content between quotes
                            printf("%.*s", (int)(len-2), arg->text + 1);
                        }
                    } else {
                        // Variable or number
                        int64_t value = eval_expression(arg);
                        printf("%lld", (long long)value);
                    }
                } else {
                    int64_t value = eval_expression(arg);
                    printf("%lld", (long long)value);
                }
            }
            printf("\n");
            return;
        }

        case AST_EXPR: {
            if (!ast->text) return;

    

            // Check if it's a variable
            if (var_exists(ast->text)) {
                get_var_value(ast->text);
                return;
            }

            // Check if it's a function call (text="call" with children)
            if (ast->text && strcmp(ast->text, "call") == 0 && ast->child_count >= 2) {
                        // Get function name from first child
        char* func_name = ast->children[0].text;
        if (func_name) {
                    
                    // Find the function
                    ASTNode* func = find_function_global(func_name);
                    if (func) {
                                            // Execute the function with arguments
                    long long result = eval_user_function_call(func, ast);
                    return;
                } else {
                    fprintf(stderr, "Error: Function '%s' not found\n", func_name);
                }
                }
            }

            // Check if it's a number
            char* endptr;
            long long num = strtoll(ast->text, &endptr, 10);
            if (*endptr == '\0') {
                return; // Number parsed successfully
            }

            // Check if it's an operator
            if (strcmp(ast->text, "+") == 0 || strcmp(ast->text, "-") == 0 ||
                strcmp(ast->text, "*") == 0 || strcmp(ast->text, "/") == 0 ||
                strcmp(ast->text, "%") == 0 || strcmp(ast->text, "==") == 0 ||
                strcmp(ast->text, "!=") == 0 || strcmp(ast->text, "<") == 0 ||
                strcmp(ast->text, ">") == 0 || strcmp(ast->text, "<=") == 0 ||
                strcmp(ast->text, ">=") == 0 || strcmp(ast->text, "and") == 0 ||
                strcmp(ast->text, "or") == 0) {
                return; // Operator nodes should not be evaluated directly
            }

            return;
        }

        case AST_LET: {
            if (ast->child_count < 2) {
                fprintf(stderr, "Error: Invalid let statement structure\n");
                return;
            }

            char* var_name = ast->children[0].text;
            if (!var_name) {
                fprintf(stderr, "Error: Variable name is NULL\n");
                return;
            }

            // Check if the value is an array literal
            if (ast->children[1].type == AST_ARRAY_LITERAL) {
                // Handle array literal creation
                if (ast->children[1].child_count == 0) {
                    // Empty array
                    MycoArray* array = create_array(8, 0); // Default capacity, numeric array
                    if (!array) {
                        fprintf(stderr, "Error: Failed to create array at line %d\n", ast->line);
                        return;
                    }
                    set_array_value(var_name, array);
                    return;
                }
                
                // Determine if this is a string array by checking the first element
                int is_string_array = 0;
                if (ast->children[1].children[0].type == AST_EXPR && ast->children[1].children[0].text && is_string_literal(ast->children[1].children[0].text)) {
                    is_string_array = 1;
                }
                
                // Create array with appropriate capacity
                MycoArray* array = create_array(ast->children[1].child_count, is_string_array);
                if (!array) {
                    fprintf(stderr, "Error: Failed to create array at line %d\n", ast->line);
                    return;
                }
                
                // Add elements to the array
                for (int i = 0; i < ast->children[1].child_count; i++) {
                    if (is_string_array) {
                        // Handle string elements
                        if (ast->children[1].children[i].type == AST_EXPR && ast->children[1].children[i].text && is_string_literal(ast->children[1].children[i].text)) {
                            // Extract string value (remove quotes)
                            size_t len = strlen(ast->children[1].children[i].text);
                            if (len >= 2) {
                                char* value = (char*)malloc(len - 1);
                                if (value) {
                                    strncpy(value, ast->children[1].children[i].text + 1, len - 2);
                                    value[len - 2] = '\0';
                                    array_push(array, value);
                                    free(value);
                                }
                            }
                        } else {
                            // Convert non-string to string
                            char temp_str[64];
                            snprintf(temp_str, sizeof(temp_str), "%lld", eval_expression(&ast->children[1].children[i]));
                            array_push(array, temp_str);
                        }
                    } else {
                        // Handle numeric elements
                        long long value = eval_expression(&ast->children[1].children[i]);
                        // Store the actual value, not a pointer to local variable
                        array->elements[array->size] = value;
                        array->size++;
                    }
                }
                
                set_array_value(var_name, array);
                return;
            }
            
            // Check if the value is an array access
            if (ast->children[1].type == AST_ARRAY_ACCESS) {
                // Handle array access: let x = arr[index]
                if (ast->children[1].child_count < 2) {
                    fprintf(stderr, "Error: Invalid array access structure at line %d\n", ast->line);
                    return;
                }
                
                // Get the array name
                char* array_name = NULL;
                if (ast->children[1].children[0].type == AST_EXPR && ast->children[1].children[0].text) {
                    array_name = ast->children[1].children[0].text;
                } else {
                    fprintf(stderr, "Error: Invalid array expression at line %d\n", ast->line);
                    return;
                }
                
                // Get the array variable
                MycoArray* array = get_array_value(array_name);
                if (!array) {
                    fprintf(stderr, "Error: Array '%s' not found at line %d\n", array_name, ast->line);
                    return;
                }
                
                // Evaluate the index expression
                long long index = eval_expression(&ast->children[1].children[1]);
                
                if (index < 0 || index >= array->size) {
                    fprintf(stderr, "Error: Array index %lld out of bounds [0, %d] at line %d\n", 
                            index, array->size - 1, ast->line);
                    return;
                }
                
                // Get the element value and assign it to the variable
                void* element = array_get(array, index);
                if (!element) {
                    fprintf(stderr, "Error: Failed to access array element at line %d\n", ast->line);
                    return;
                }
                
                if (array->is_string_array) {
                    set_str_value(var_name, (char*)element);
                } else {
                    long long num_value = *(long long*)element;
                    set_var_value(var_name, num_value);
                }
                return;
            }
            
            // Handle regular numeric assignment
            int64_t value = eval_expression(&ast->children[1]);
            set_var_value(var_name, value);
            return;
        }

        case AST_ASSIGN: {
            if (ast->child_count < 2) {
                fprintf(stderr, "Error: Invalid assignment statement structure\n");
                return;
            }

            char* var_name = ast->children[0].text;
            if (!var_name) {
                fprintf(stderr, "Error: Variable name is NULL\n");
                return;
            }

            int64_t value = eval_expression(&ast->children[1]);
            set_var_value(var_name, value);
            return;
        }





        case AST_ARRAY_ASSIGN: {
            if (ast->child_count < 3) {
                fprintf(stderr, "Error: Invalid array assignment structure\n");
                return;
            }
            
            // Get the array name from the first child
            char* array_name = ast->children[0].text;
            if (!array_name) {
                fprintf(stderr, "Error: Array name is NULL\n");
                return;
            }
            

            
            // Get the array variable
            MycoArray* array = get_array_value(array_name);
            if (!array) {
                fprintf(stderr, "Error: Array '%s' not found at line %d\n", array_name, ast->line);
                return;
            }

            
            // Evaluate the index expression
            long long index = eval_expression(&ast->children[1]);

            if (index < 0 || index >= array->size) {
                fprintf(stderr, "Error: Array index %lld out of bounds [0, %d] at line %d\n", 
                        index, array->size - 1, ast->line);
                return;
            }
            
            // Evaluate the value expression
            long long value = eval_expression(&ast->children[2]);

            
            // Set the array element

            if (!array_set(array, index, &value)) {
                fprintf(stderr, "Error: Failed to set array element at line %d\n", ast->line);
                return;
            }

            
            return;
        }

        case AST_IF: {
            if (ast->child_count < 2) {
                fprintf(stderr, "Error: Invalid if statement structure\n");
                return;
            }

            int64_t condition = eval_expression(&ast->children[0]);
            if (condition) {
                eval_evaluate(&ast->children[1]);
            } else if (ast->child_count > 2) {
                eval_evaluate(&ast->children[2]);
            }
            return;
        }

        case AST_RETURN: {
            if (ast->child_count > 0) {
                int64_t value = eval_expression(&ast->children[0]);
                
                // Set global return values for function calls
                return_flag = 1;
                return_value = value;
                
                if (global_loop_state) {
                    global_loop_state->return_requested = 1;
                }
                return;
            }
            
            // Set global return values for function calls
            return_flag = 1;
            return_value = 0;
            
            if (global_loop_state) {
                global_loop_state->return_requested = 1;
            }
            return;
        }

        case AST_FUNC: {
            // Function definition - register it in the function registry
            if (ast->text && ast->child_count >= 2) {
                // Register the function for later calls
                register_function(ast->text, ast);
            }
            return;
        }

        default:
            fprintf(stderr, "Error: Unhandled AST node type: %d\n", ast->type);
            return;
    }
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
    
    // Reset scope stack
    if (scope_stack) {
        free(scope_stack);
        scope_stack = NULL;
    }
    scope_stack_size = 0;
    scope_stack_capacity = 0;
    
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

// Error handling with conditional compilation
static void print_error(const char* message, int line) {
    #if ENABLE_DETAILED_ERRORS
    char error_msg[512];
    snprintf(error_msg, sizeof(error_msg), "Line %d: %s", line, message);
    fprintf(stderr, "%s\n", error_msg);
    #else
    fprintf(stderr, "Error at line %d\n", line);
    #endif
}

// Print function with conditional debug output
static void eval_print(ASTNode* ast) {
    #if DEBUG_EVAL_TRACE
    
    #endif
    
    for (int i = 0; i < ast->child_count; i++) {
        ASTNode* child = &ast->children[i];
        
        // Check if it's a string literal (text starts with quote)
        if (child->text && child->text[0] == '"') {
            // Print string literal
            size_t len = strlen(child->text);
            if (len > 2) {
                printf("%.*s", (int)(len-2), child->text+1);
            }
        } else {
            // Print variable value
            const char* sv = get_str_value(child->text);
            if (sv) { 
                printf("%s", sv); 
                goto after_print_arg; 
            }
            
            // For non-string nodes, evaluate them
            eval_evaluate(child);
        }
        
        after_print_arg:
        if (i < ast->child_count - 1) {
            printf(" ");
        }
    }
    printf("\n");
}