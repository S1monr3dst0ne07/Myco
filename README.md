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

To compile a Myco source file:

```bash
./bin/myco example.myco > output.c
gcc output.c -o program
./program
```

## Example

```myco
# Showcase of Myco logic and features

const PI = 3.1415
let name = "Ivy"
let age = 15
let score: float = 88.5

print("Welcome, " .. name)
print("Your age is:", age)
print("Score:", score)

if age >= 18:
    print("Adult")
else:
    print("Minor")
end

func greet(user):
    return "Hi, " .. user
end

print(greet(name))

func factorial(n: int) -> int:
    let result = 1
    for let i = 1; i++; i <= n:
        result *= i
    end
    return result
end

print("5! =", factorial(5))

let fruits = ["apple", "banana", "cherry"]
for fruit in fruits:
    print("Fruit:", fruit)
end

switch age:
    case 13:
        print("Teen")
    case 15:
        print("Sophomore")
    else:
        print("Other")
end

try:
    let data = read_file("file.txt")
    print("File:", data)
catch err:
    print("Failed to read file:", err)
end

func test_logic(a: int, b: int):
    if a == b:
        print("Equal")
    elseif a > b:
        print("A is greater")
    else:
        print("B is greater")
    end
end

test_logic(10, 20)

a, b = 5, 10
b, a = a, b  # swap
print("a:", a, "b:", b)
```

## License

MIT License 