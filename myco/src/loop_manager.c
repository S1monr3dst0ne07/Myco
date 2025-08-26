/**
 * @file loop_manager.c
 * @brief Myco Loop Management System - Safe Loop Execution
 * @version 1.0.0
 * @author Myco Development Team
 * 
 * This file implements a comprehensive loop management system that ensures
 * safe execution of loops in the Myco interpreter. It provides protection
 * against infinite loops, excessive resource consumption, and stack overflow.
 * 
 * Loop Safety Features:
 * - Maximum iteration limits to prevent infinite loops
 * - Maximum loop depth to prevent stack overflow
 * - Range validation to ensure loop termination
 * - Step value validation to prevent zero-step loops
 * - Loop context management and cleanup
 * - Break, continue, and return flow control
 * 
 * Loop Types Supported:
 * - For loops with range and step
 * - While loops with condition checking
 * - Nested loops with depth tracking
 * - Loop variable management
 * 
 * Safety Mechanisms:
 * - Iteration counting and limits
 * - Memory-efficient context storage
 * - Automatic cleanup on loop exit
 * - Error reporting and warnings
 * - Cross-platform compatibility
 */

#define _POSIX_C_SOURCE 200809L
#include "loop_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_tracker.h"
#include <inttypes.h>

/*******************************************************************************
 * GLOBAL LOOP STATISTICS
 ******************************************************************************/

/**
 * Global statistics tracking for all loops executed in the program.
 * This provides insights into loop behavior and helps identify
 * potential performance issues or infinite loop patterns.
 */
static LoopStatistics global_loop_stats = {0};

/*******************************************************************************
 * LOOP CONTEXT MANAGEMENT
 ******************************************************************************/

/**
 * @brief Creates a new loop execution context
 * @param var_name Name of the loop variable
 * @param start Starting value for the loop
 * @param end Ending value for the loop
 * @param step Step value between iterations
 * @param line Source line number for error reporting
 * @return New loop context, or NULL on failure
 * 
 * A loop context contains all the information needed to execute a loop:
 * - Loop variable name and current value
 * - Range boundaries and step size
 * - Iteration counting and limits
 * - Parent loop reference for nesting
 * - Source location for debugging
 * 
 * The context is validated to ensure safe execution:
 * - Step value cannot be zero
 * - Range size is within safe limits
 * - Loop will terminate naturally
 */
LoopContext* create_loop_context(const char* var_name, int64_t start, int64_t end, int64_t step, int line) {
    LoopContext* context = (LoopContext*)tracked_malloc(sizeof(LoopContext), __FILE__, __LINE__, "loop_manager_context");
    if (!context) {
        fprintf(stderr, "Error: Failed to allocate loop context\n");
        return NULL;
    }
    
    // Initialize context
    context->loop_var_name = var_name ? tracked_strdup(var_name, __FILE__, __LINE__, "loop_manager_var_name") : NULL;
    context->current_value = start;
    context->start_value = start;
    context->end_value = end;
    context->step_value = step;
    context->iteration_count = 0;
    context->max_iterations = MAX_LOOP_ITERATIONS;
    context->line = line;
    context->parent = NULL;
    
    // Validate range
    if (!validate_loop_range(start, end, step)) {
        fprintf(stderr, "Warning: Invalid loop range at line %d: %" PRId64 " to %" PRId64 " step %" PRId64 "\n", 
                line, start, end, step);
    }
    
    return context;
}

/**
 * @brief Destroys a loop context and frees associated memory
 * @param context The loop context to destroy
 * 
 * Performs cleanup operations:
 * - Frees the loop variable name string
 * - Frees the context structure itself
 * - Handles NULL contexts gracefully
 * 
 * This function should be called when a loop exits
 * to prevent memory leaks.
 */
void destroy_loop_context(LoopContext* context) {
    if (!context) return;
    
    if (context->loop_var_name) {
        tracked_free((void*)context->loop_var_name, __FILE__, __LINE__, "loop_manager_cleanup_var_name");
    }
    tracked_free(context, __FILE__, __LINE__, "loop_manager_cleanup_context");
}

/**
 * @brief Validates loop range parameters for safety
 * @param start Starting value for the loop
 * @param end Ending value for the loop
 * @param step Step value between iterations
 * @return 1 if valid, 0 if invalid
 * 
 * Performs comprehensive validation to ensure loop safety:
 * - Prevents zero-step infinite loops
 * - Ensures range size is within safe limits
 * - Verifies loop will terminate naturally
 * - Checks for positive/negative step compatibility
 * 
 * Safety checks:
 * - Step cannot be zero (infinite loop prevention)
 * - Range size must be within MAX_LOOP_RANGE
 * - Step size must be within MAX_LOOP_RANGE
 * - Loop direction must match step direction
 */
int validate_loop_range(int64_t start, int64_t end, int64_t step) {
    if (step == 0) return 0;  // Prevent infinite loops
    if (llabs(end - start) > MAX_LOOP_RANGE) return 0;
    if (llabs(step) > MAX_LOOP_RANGE) return 0;
    
    // Check if loop will terminate
    if (step > 0 && start > end) return 0;  // Positive step, start > end
    if (step < 0 && start < end) return 0;  // Negative step, start < end
    
    return 1;
}

/*******************************************************************************
 * LOOP EXECUTION STATE MANAGEMENT
 ******************************************************************************/

/**
 * @brief Creates a new loop execution state manager
 * @return New loop execution state, or NULL on failure
 * 
 * The execution state manages the global loop execution environment:
 * - Tracks active loops in a stack structure
 * - Manages loop depth and nesting limits
 * - Handles break, continue, and return requests
 * - Provides loop body execution state
 * - Maintains loop context hierarchy
 * 
 * This state is shared across all loops in the program
 * and ensures proper loop flow control.
 */
LoopExecutionState* create_loop_execution_state(void) {
    LoopExecutionState* state = (LoopExecutionState*)tracked_malloc(sizeof(LoopExecutionState), __FILE__, __LINE__, "loop_manager_state");
    if (!state) {
        fprintf(stderr, "Error: Failed to allocate loop execution state\n");
        return NULL;
    }
    
    // Initialize state
    state->active_loops = (LoopContext*)tracked_malloc(MAX_LOOP_DEPTH * sizeof(LoopContext*), __FILE__, __LINE__, "loop_manager_active_loops");
    if (!state->active_loops) {
        tracked_free(state, __FILE__, __LINE__, "loop_manager_error_cleanup");
        return NULL;
    }
    
    state->loop_stack_size = 0;
    state->max_loop_depth = MAX_LOOP_DEPTH;
    state->in_loop_body = 0;
    state->break_requested = 0;
    state->continue_requested = 0;
    state->return_requested = 0;
    
    return state;
}

// Destroy loop execution state
void destroy_loop_execution_state(LoopExecutionState* state) {
    if (!state) return;
    
    // Clean up any remaining loop contexts
    while (state->loop_stack_size > 0) {
        LoopContext* context = pop_loop_context(state);
        if (context) destroy_loop_context(context);
    }
    
    if (state->active_loops) {
        tracked_free(state->active_loops, __FILE__, __LINE__, "loop_manager_cleanup_active_loops");
    }
    tracked_free(state, __FILE__, __LINE__, "loop_manager_cleanup_state");
}

// Push a loop context onto the stack
void push_loop_context(LoopExecutionState* state, LoopContext* context) {
    if (!state || !context || state->loop_stack_size >= state->max_loop_depth) {
        fprintf(stderr, "Error: Cannot push loop context - stack full or invalid state\n");
        return;
    }
    
    // Set parent pointer for nested loops
    if (state->loop_stack_size > 0) {
        context->parent = &state->active_loops[state->loop_stack_size - 1];
    }
    
    state->active_loops[state->loop_stack_size] = *context;
    state->loop_stack_size++;
    
    // Update statistics
    global_loop_stats.max_loop_depth_reached = 
        (state->loop_stack_size > global_loop_stats.max_loop_depth_reached) ? 
        state->loop_stack_size : global_loop_stats.max_loop_depth_reached;
}

// Pop a loop context from the stack
LoopContext* pop_loop_context(LoopExecutionState* state) {
    if (!state || state->loop_stack_size <= 0) {
        return NULL;
    }
    
    state->loop_stack_size--;
    LoopContext* context = &state->active_loops[state->loop_stack_size];
    
    // Create a copy to return (since the stack entry will be overwritten)
    LoopContext* copy = create_loop_context(
        context->loop_var_name,
        context->start_value,
        context->end_value,
        context->step_value,
        context->line
    );
    
    if (copy) {
        copy->current_value = context->current_value;
        copy->iteration_count = context->iteration_count;
    }
    
    return copy;
}

// Get current loop context
LoopContext* get_current_loop_context(LoopExecutionState* state) {
    if (!state || state->loop_stack_size <= 0) {
        return NULL;
    }
    return &state->active_loops[state->loop_stack_size - 1];
}

// Log loop execution
void log_loop_execution(LoopContext* context) {
    if (!context) return;
    
    printf("Loop execution - %s: %" PRId64 " to %" PRId64 " step %" PRId64 " (line %d)\n",
           context->loop_var_name ? context->loop_var_name : "unknown",
           context->start_value, context->end_value, context->step_value, context->line);
}

// Print loop statistics
void print_loop_statistics(void) {
    printf("\n=== Loop Execution Statistics ===\n");
    printf("Total loops executed: %d\n", global_loop_stats.total_loops_executed);
    printf("Total iterations: %d\n", global_loop_stats.total_iterations);
    printf("Max iterations in single loop: %d\n", global_loop_stats.max_iterations_in_single_loop);
    printf("Loops with errors: %d\n", global_loop_stats.loops_with_errors);
    printf("Max loop depth reached: %d\n", global_loop_stats.max_loop_depth_reached);
    printf("================================\n\n");
}

// Get loop statistics
LoopStatistics* get_loop_statistics(void) {
    return &global_loop_stats;
}

// Update loop statistics
void update_loop_statistics(int loops_executed, int iterations, int had_errors) {
    global_loop_stats.total_loops_executed += loops_executed;
    global_loop_stats.total_iterations += iterations;
    global_loop_stats.loops_with_errors += had_errors;
    
    if (iterations > global_loop_stats.max_iterations_in_single_loop) {
        global_loop_stats.max_iterations_in_single_loop = iterations;
    }
}
