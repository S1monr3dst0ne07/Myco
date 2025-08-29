#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// High-precision timing function
long long get_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

// Benchmark timing structure
typedef struct {
    long long start_time;
    long long end_time;
    long long duration;
} BenchmarkTimer;

void start_benchmark(BenchmarkTimer* timer) {
    timer->start_time = get_time_microseconds();
}

long long end_benchmark(BenchmarkTimer* timer) {
    timer->end_time = get_time_microseconds();
    timer->duration = timer->end_time - timer->start_time;
    return timer->duration;
}

// Dynamic array implementation (similar to Myco)
typedef struct {
    int* data;
    int size;
    int capacity;
} DynamicArray;

void init_array(DynamicArray* arr, int initial_capacity) {
    arr->data = malloc(initial_capacity * sizeof(int));
    arr->size = 0;
    arr->capacity = initial_capacity;
}

void push_array(DynamicArray* arr, int value) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(int));
    }
    arr->data[arr->size++] = value;
}

void free_array(DynamicArray* arr) {
    free(arr->data);
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

// Dynamic string implementation (similar to Myco)
typedef struct {
    char* data;
    int size;
    int capacity;
} DynamicString;

void init_string(DynamicString* str, int initial_capacity) {
    str->data = malloc(initial_capacity);
    str->data[0] = '\0';
    str->size = 0;
    str->capacity = initial_capacity;
}

void append_string(DynamicString* str, const char* value) {
    int value_len = strlen(value);
    if (str->size + value_len + 1 >= str->capacity) {
        str->capacity = (str->size + value_len + 1) * 2;
        str->data = realloc(str->data, str->capacity);
    }
    strcat(str->data, value);
    str->size += value_len;
}

void free_string(DynamicString* str) {
    free(str->data);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}

// Fibonacci function (identical to Myco)
long long fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Bubble sort (identical to Myco)
void bubble_sort(int* arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int main() {
    printf("=== C STANDARDIZED BENCHMARK SUITE ===\n");
    printf("Testing identical operations across all languages\n\n");
    
    BenchmarkTimer timer;
    long long loop_time, string_time, array_time, math_time, func_time;
    long long nested_time, search_time, sort_time, recursive_time, memory_time;
    
    // BENCHMARK 1: Simple Loop (1M iterations) - IDENTICAL to Myco
    start_benchmark(&timer);
    long long sum = 0;
    for (long long i = 1; i <= 1000000; i++) {
        sum += i;
    }
    loop_time = end_benchmark(&timer);
    printf("Simple Loop (1M): %lld microseconds, Sum: %lld\n", loop_time, sum);
    
    // BENCHMARK 2: String Concatenation (10K) - IDENTICAL to Myco
    start_benchmark(&timer);
    DynamicString str_result;
    init_string(&str_result, 1000);
    int total_len = 0;
    
    for (int i = 1; i <= 10000; i++) {
        char num_str[20];
        sprintf(num_str, "%d", i);
        append_string(&str_result, num_str);
        total_len += strlen(num_str);
    }
    string_time = end_benchmark(&timer);
    printf("String Concatenation (10K): %lld microseconds, Length: %d\n", string_time, total_len);
    free_string(&str_result);
    
    // BENCHMARK 3: Array Creation (100K) - IDENTICAL to Myco
    start_benchmark(&timer);
    DynamicArray arr;
    init_array(&arr, 1000);
    
    for (int i = 1; i <= 100000; i++) {
        push_array(&arr, i);
    }
    array_time = end_benchmark(&timer);
    printf("Array Creation (100K): %lld microseconds, Length: %d\n", array_time, arr.size);
    free_array(&arr);
    
    // BENCHMARK 4: Math Operations (100K) - IDENTICAL to Myco
    start_benchmark(&timer);
    long long result = 0;
    for (long long i = 1; i <= 100000; i++) {
        result += (i * i) / 2;
    }
    math_time = end_benchmark(&timer);
    printf("Math Operations (100K): %lld microseconds, Result: %lld\n", math_time, result);
    
    // BENCHMARK 5: Function Calls (100K) - IDENTICAL to Myco
    start_benchmark(&timer);
    for (long long i = 1; i <= 100000; i++) {
        result = i * i;  // Same as lambda function
    }
    func_time = end_benchmark(&timer);
    printf("Function Calls (100K): %lld microseconds\n", func_time);
    
    // BENCHMARK 6: Nested Loop (1K x 1K) - IDENTICAL to Myco
    start_benchmark(&timer);
    long long nested_sum = 0;
    for (int i = 1; i <= 1000; i++) {
        for (int j = 1; j <= 1000; j++) {
            nested_sum += i + j;
        }
    }
    nested_time = end_benchmark(&timer);
    printf("Nested Loop (1K x 1K): %lld microseconds, Sum: %lld\n", nested_time, nested_sum);
    
    // BENCHMARK 7: String Search (100K) - IDENTICAL to Myco
    start_benchmark(&timer);
    DynamicString search_text;
    init_string(&search_text, 1000);
    
    for (int i = 1; i <= 100000; i++) {
        append_string(&search_text, "abc");
    }
    
    int search_count = 0;
    for (int i = 1; i <= 1000; i++) {
        if (strstr(search_text.data, "abc") != NULL) {
            search_count++;
        }
    }
    search_time = end_benchmark(&timer);
    printf("String Search (100K): %lld microseconds, Found: %d\n", search_time, search_count);
    free_string(&search_text);
    
    // BENCHMARK 8: Array Sorting (10K) - IDENTICAL to Myco
    start_benchmark(&timer);
    int* sort_arr = malloc(10000 * sizeof(int));
    for (int i = 0; i < 10000; i++) {
        sort_arr[i] = 10000 - i;
    }
    
    bubble_sort(sort_arr, 10000);
    sort_time = end_benchmark(&timer);
    printf("Array Sorting (10K): %lld microseconds, First: %d, Last: %d\n", 
           sort_time, sort_arr[0], sort_arr[9999]);
    free(sort_arr);
    
    // BENCHMARK 9: Recursive Functions (1K) - IDENTICAL to Myco
    start_benchmark(&timer);
    long long fib_result = fibonacci(20);
    recursive_time = end_benchmark(&timer);
    printf("Recursive Functions (1K): %lld microseconds, Fib(20): %lld\n", recursive_time, fib_result);
    
    // BENCHMARK 10: Memory Operations (10K) - IDENTICAL to Myco
    start_benchmark(&timer);
    DynamicArray mem_arr;
    init_array(&mem_arr, 1000);
    
    for (int i = 1; i <= 10000; i++) {
        push_array(&mem_arr, i);
        if (mem_arr.size > 5000) {
            free_array(&mem_arr);
            init_array(&mem_arr, 1000);
        }
    }
    memory_time = end_benchmark(&timer);
    printf("Memory Operations (10K): %lld microseconds, Final Length: %d\n", memory_time, mem_arr.size);
    free_array(&mem_arr);
    
    printf("\n=== C BENCHMARK RESULTS ===\n");
    printf("Simple Loop (1M): %lld microseconds\n", loop_time);
    printf("String Concatenation (10K): %lld microseconds\n", string_time);
    printf("Array Creation (100K): %lld microseconds\n", array_time);
    printf("Math Operations (100K): %lld microseconds\n", math_time);
    printf("Function Calls (100K): %lld microseconds\n", func_time);
    printf("Nested Loop (1K x 1K): %lld microseconds\n", nested_time);
    printf("String Search (100K): %lld microseconds\n", search_time);
    printf("Array Sorting (10K): %lld microseconds\n", sort_time);
    printf("Recursive Functions (1K): %lld microseconds\n", recursive_time);
    printf("Memory Operations (10K): %lld microseconds\n", memory_time);
    
    long long total_time = loop_time + string_time + array_time + math_time + func_time + 
                           nested_time + search_time + sort_time + recursive_time + memory_time;
    printf("\nTotal Benchmark Time: %lld microseconds\n", total_time);
    printf("Total Benchmark Time: %.1f milliseconds\n", total_time / 1000.0);
    
    return 0;
}
