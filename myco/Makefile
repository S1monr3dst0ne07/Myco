CC = gcc
CFLAGS = -O2 -std=c99 -Iinclude
SRC = src/main.c src/lexer.c src/parser.c src/eval.c src/codegen.c src/memory_tracker.c
OUT = myco
WINCC = x86_64-w64-mingw32-gcc
WINOUT = myco.exe

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

windows:
	$(WINCC) -O2 -std=c99 -Iinclude -o $(WINOUT) $(SRC) -lws2_32 -lcrypt32

clean:
	rm -f $(OUT) $(WINOUT) output.c 