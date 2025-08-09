CC = gcc
CFLAGS = -O2 -std=c99 -Iinclude
SRC = src/main.c src/lexer.c src/parser.c src/eval.c src/codegen.c
OUT = myco

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT) output.c 