#ifndef LOOP_MANAGER_H
#define LOOP_MANAGER_H

#include <stdint.h>

// Loop execution context
typedef struct LoopContext {
    const char* loop_var_name;    // Loop variable identifier
    int64_t current_value;        // Current iteration value
    int64_t start_value;          // Range start
    int64_t end_value;            // Range end
    int64_t step_value;           // Step increment (default: 1)
    int iteration_count;          // Safety counter
    int max_iterations;           // Maximum allowed iterations
    int line;                     // Source line number
    struct LoopContext* parent;   // For nested loops
} LoopContext;

// Loop execution state
typedef struct {
    LoopContext* active_loops;    // Stack of active loops
    int loop_stack_size;          // Current stack depth
    int max_loop_depth;           // Maximum allowed nesting
    int in_loop_body;             // Flag: currently executing loop body
    int break_requested;          // Flag: break statement encountered
    int continue_requested;        // Flag: continue statement encountered
    int return_requested;          // Flag: return statement encountered
} LoopExecutionState;

// Loop statistics
typedef struct {
    int total_loops_executed;
    int total_iterations;
    int max_iterations_in_single_loop;
    int loops_with_errors;
    int max_loop_depth_reached;
} LoopStatistics;

// Function prototypes
LoopContext* create_loop_context(const char* var_name, int64_t start, int64_t end, int64_t step, int line);
void destroy_loop_context(LoopContext* context);
int validate_loop_range(int64_t start, int64_t end, int64_t step);

LoopExecutionState* create_loop_execution_state(void);
void destroy_loop_execution_state(LoopExecutionState* state);
void push_loop_context(LoopExecutionState* state, LoopContext* context);
LoopContext* pop_loop_context(LoopExecutionState* state);
LoopContext* get_current_loop_context(LoopExecutionState* state);

void log_loop_execution(LoopContext* context);
void print_loop_statistics(void);
LoopStatistics* get_loop_statistics(void);
void update_loop_statistics(int loops_executed, int iterations, int had_errors);

// Safety constants
#define MAX_LOOP_ITERATIONS 1000000
#define MAX_LOOP_DEPTH 100
#define MAX_LOOP_RANGE 1000000

#endif // LOOP_MANAGER_H
