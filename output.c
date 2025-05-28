#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Runtime functions
void myco_print(const char* str) { printf("%s", str); }
void myco_print_int(int64_t value) { printf("%lld", value); }
void myco_print_float(double value) { printf("%g", value); }
void myco_print_bool(bool value) { printf(value ? "true" : "false"); }
char* myco_str_concat(const char* a, const char* b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = malloc(len_a + len_b + 1);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

 x = 
10;

int main() {
    return 0;
}
