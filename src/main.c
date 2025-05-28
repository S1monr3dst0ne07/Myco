#include <stdio.h>
#include <stdlib.h>
#include "myco.h"

static void run_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        return;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* src = malloc(len + 1);
    fread(src, 1, len, f);
    src[len] = '\0';
    fclose(f);
    Node* root = parse(src);
    interpret(root);
    free(src);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: myco [script]\n");
        exit(64);
    }
    run_file(argv[1]);
    return 0;
} 