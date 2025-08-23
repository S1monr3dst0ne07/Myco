/**
 * @file eval.h
 * @brief Myco Core Evaluation Header - Core evaluation function declarations
 * @version 1.6.0
 * @author Myco Development Team
 */

#ifndef CORE_EVAL_H
#define CORE_EVAL_H

#include "parser.h"

// Core evaluation function
long long eval_expression(ASTNode* ast);

// Variable management functions
void set_variable(const char* name, long long value);
long long get_variable(const char* name);

// Function management functions
long long call_user_function(const char* name, ASTNode* ast);

// Built-in function evaluations
long long eval_print_function(ASTNode* ast);
long long eval_len_function(ASTNode* ast);
long long eval_first_function(ASTNode* ast);
long long eval_last_function(ASTNode* ast);
long long eval_push_function(ASTNode* ast);
long long eval_pop_function(ASTNode* ast);
long long eval_reverse_function(ASTNode* ast);
long long eval_filter_function(ASTNode* ast);
long long eval_map_function(ASTNode* ast);
long long eval_reduce_function(ASTNode* ast);

// Library function calls
long long call_math_function(const char* func_name, ASTNode* args_node);
long long call_util_function(const char* func_name, ASTNode* args_node);
long long call_file_io_function(const char* func_name, ASTNode* args_node);
long long call_path_utils_function(const char* func_name, ASTNode* args_node);
long long call_env_function(const char* func_name, ASTNode* args_node);
long long call_args_function(const char* func_name, ASTNode* args_node);
long long call_process_function(const char* func_name, ASTNode* args_node);
long long call_text_utils_function(const char* func_name, ASTNode* args_node);
long long call_debug_function(const char* func_name, ASTNode* args_node);
long long call_type_system_function(const char* func_name, ASTNode* args_node);
long long call_language_polish_function(const char* func_name, ASTNode* args_node);
long long call_testing_framework_function(const char* func_name, ASTNode* args_node);
long long call_data_structures_function(const char* func_name, ASTNode* args_node);

// Library import functions
const char* get_library_alias(const char* library_name);
MycoArray* get_array_value(const char* name);
MycoObject* get_object_value(const char* name);

#endif // CORE_EVAL_H
