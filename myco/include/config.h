#ifndef MYCO_CONFIG_H
#define MYCO_CONFIG_H

// Build configuration macros
#ifdef NDEBUG
    #define MYCO_RELEASE 1
    #define MYCO_DEBUG 0
#else
    #define MYCO_RELEASE 0
    #define MYCO_DEBUG 1
#endif

// Performance optimization macros
#ifdef MYCO_RELEASE
    // Release mode: maximum performance, minimal size
    #define DEBUG_PRINT(fmt, ...) ((void)0)
    #define DEBUG_MEMORY_TRACKING 0
    #define DEBUG_AST_VALIDATION 0
    #define DEBUG_LEXER_TRACE 0
    #define DEBUG_PARSER_TRACE 0
    #define DEBUG_EVAL_TRACE 0
    
    // Inline critical functions for maximum speed
    #define INLINE_CRITICAL static inline
    #define FAST_STRING_COMPARE(str1, str2) (str1 == str2 || strcmp(str1, str2) == 0)
    #define FAST_NUMBER_CHECK(str) (str[0] >= '0' && str[0] <= '9')
    
    // Disable expensive debug features
    #define ENABLE_DETAILED_ERRORS 0
    #define ENABLE_MEMORY_STATS 0
    #define ENABLE_PERFORMANCE_PROFILING 0
#else
    // Debug mode: full debugging, error checking
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define DEBUG_MEMORY_TRACKING 1
    #define DEBUG_AST_VALIDATION 1
    #define DEBUG_LEXER_TRACE 1
    #define DEBUG_PARSER_TRACE 1
    #define DEBUG_EVAL_TRACE 1
    
    // No inlining in debug mode for better debugging
    #define INLINE_CRITICAL static
    #define FAST_STRING_COMPARE(str1, str2) strcmp(str1, str2) == 0
    #define FAST_NUMBER_CHECK(str) (str[0] >= '0' && str[0] <= '9')
    
    // Enable all debug features
    #define ENABLE_DETAILED_ERRORS 1
    #define ENABLE_MEMORY_STATS 1
    #define ENABLE_PERFORMANCE_PROFILING 1
#endif

// Platform-specific optimizations
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_UNIX 0
    #define USE_WINDOWS_APIS 1
    #define USE_UNIX_APIS 0
#elif defined(__APPLE__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_UNIX 1
    #define USE_WINDOWS_APIS 0
    #define USE_UNIX_APIS 1
    #ifdef __arm64__
        #define OPTIMIZE_FOR_ARM64 1
        #define USE_APPLE_APIS 1
    #else
        #define OPTIMIZE_FOR_ARM64 0
        #define USE_APPLE_APIS 0
    #endif
#else
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_UNIX 1
    #define USE_WINDOWS_APIS 0
    #define USE_UNIX_APIS 1
    #define OPTIMIZE_FOR_ARM64 0
    #define USE_APPLE_APIS 0
#endif

// Architecture-specific optimizations
#ifdef __x86_64__
    #define ARCH_X86_64 1
    #define ARCH_ARM64 0
#elif defined(__aarch64__) || defined(__arm64__)
    #define ARCH_X86_64 0
    #define ARCH_ARM64 1
#else
    #define ARCH_X86_64 0
    #define ARCH_ARM64 0
#endif

// Compiler-specific optimizations
#ifdef __GNUC__
    #define COMPILER_GCC 1
    #if __GNUC__ >= 8
        #define SUPPORT_LTO 1
        #define SUPPORT_FUNCTION_SECTIONS 1
    #else
        #define SUPPORT_LTO 0
        #define SUPPORT_FUNCTION_SECTIONS 0
    #endif
#else
    #define COMPILER_GCC 0
    #define SUPPORT_LTO 0
    #define SUPPORT_FUNCTION_SECTIONS 0
#endif

// Feature flags based on build configuration
#define ENABLE_HTTP_FEATURES 1
#define ENABLE_DISCORD_FEATURES 1
#define ENABLE_MODULE_SYSTEM 1
#define ENABLE_MEMORY_TRACKING (DEBUG_MEMORY_TRACKING || ENABLE_MEMORY_STATS)

// Performance tuning constants
#ifdef MYCO_RELEASE
    #define INITIAL_STRING_CAPACITY 32
    #define INITIAL_ARRAY_CAPACITY 8
    #define MAX_ERROR_MESSAGE_LENGTH 128
    #define MAX_DEBUG_OUTPUT_LENGTH 64
#else
    #define INITIAL_STRING_CAPACITY 64
    #define INITIAL_ARRAY_CAPACITY 16
    #define MAX_ERROR_MESSAGE_LENGTH 512
    #define MAX_DEBUG_OUTPUT_LENGTH 256
#endif

// Memory management tuning
#define MEMORY_POOL_SIZE 4096
#define MAX_MEMORY_TRACKING_ENTRIES 10000

#endif // MYCO_CONFIG_H
