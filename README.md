# Myco Programming Language

Myco is a modern, Python-inspired programming language that compiles to C. It features a clean syntax, optional type annotations, and modern language features.

## Features

- Clean, Python-inspired syntax
- Optional type annotations
- Modern control structures (if/else, for, switch, try/catch)
- String concatenation with `..`
- Multi-assignment support
- Built-in print function
- Comments with `#`

## Building

To build the Myco compiler:

```bash
make
```

This will create the compiler in the `bin` directory.

## Usage

To execute a Myco source file:

```bash
./bin/myco example.myco
```

To compile a Myco source file into an executable:

```bash
./bin/myco --build example.myco
```

## Example

```myco
# Showcase of Myco logic and features

const PI = 3.1415;
let name = "Ivy";
let age = 15;
let score: float = 88.5;

print("Welcome, " .. name);
print("Your age is:", age);
print("Score:", score);

if age >= 18:
    print("Adult");
else:
    print("Minor");
end

func greet(user):
    return "Hi, " .. user;
end

print(greet(name));

func factorial(n: int) -> int:
    let result = 1;
    for let i = 1; i++; i <= n:
        result *= i;
    end
    return result;
end

print("5! =", factorial(5));

let fruits = ["apple", "banana", "cherry"];
for fruit in fruits:
    print("Fruit:", fruit);
end

switch age:
    case 13:
        print("Teen");
    case 15:
        print("Sophomore");
    else:
        print("Other");
end

try:
    let data = read_file("file.txt");
    print("File:", data);
catch err:
    print("Failed to read file:", err);
end

func test_logic(a: int, b: int):
    if a == b:
        print("Equal");
    elseif a > b:
        print("A is greater");
    else:
        print("B is greater");
    end
end

test_logic(10, 20);

a, b = 5, 10;
b, a = a, b;  # swap
print("a:", a, "b:", b);
```

## Language Features

### Variable Declarations

```myco
let x = 10;              # Immutable variable
var y = 20;              # Mutable variable
const z = 30;            # Constant
let typed: int = 42;     # With type annotation
```

### Functions

```myco
func add(a, b):
    return a + b;
end

func typed_func(x: int, y: int) -> int:
    return x + y;
end
```

### Control Structures

```myco
if condition:
    # code
elseif other_condition:
    # code
else:
    # code
end

while condition:
    # code
end

for item in items:
    # code
end

switch value:
    case 1:
        # code
    case 2:
        # code
    else:
        # code
end
```

### Data Structures

```myco
let list = [1, 2, 3];
let map = {"key": "value"};
```

### Error Handling

```myco
try:
    # code that might fail
catch err:
    # handle error
end
```

## Implementation Details

The Myco compiler is implemented in C and consists of several components:

1. Lexer (`lexer.h`, `lexer.c`): Tokenizes the input source code
2. Parser (`parser.h`, `parser.c`): Builds an Abstract Syntax Tree (AST)
3. Code Generator (`codegen.h`, `codegen.c`): Generates C code from the AST
4. Runtime Library (`runtime.c`): Provides runtime support for Myco features

The compiler follows these steps:

1. Read the input `.myco` file
2. Tokenize the input into tokens
3. Parse the tokens into an AST
4. Generate C code from the AST
5. (Optional) Compile the generated C code into an executable

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 