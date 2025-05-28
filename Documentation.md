# Myco Programming Language Reference

## Variable Declarations

Myco supports multiple ways to declare and manipulate variables with flexibility and type safety.

### Implicit Declaration

```myco
x = 10
name = "Ivy"
```

* Type is inferred from the value.

### Explicit Declaration

```myco
let x: int = 10
let name: str = "Ivy"
```

* Type is specified using `:`.
* Supported types: `int`, `float`, `str`, `bool`, `list`, `map`, `none`

### Constants

```myco
const PI = 3.14
```

* `const` variables cannot be reassigned.

### Changing Variables

```myco
x = 5
x = x + 1
x++
x--
```

* Arithmetic and post-increment/decrement are supported.

### Multiple Variable Assignment

```myco
let a, b = 1, 2
let x: int, y: int = 5, 10
const width, height = 1920, 1080
let name, age, is_admin = "Ivy", 15, false
```

* Declare multiple variables in one line.
* Works with or without type annotations.
* Tuple unpacking supported.

## String Concatenation

```myco
let name = "Ivy"
print("Hello, " .. name)
print("Age: " + str(15))
print("User:", name)  # comma-separated
```

* `..` for native concat, `+` for casted concat, `,` in print.

## Modules and Imports

```myco
import "utils.myco"
import "math.myco"
```

* Imports another `.myco` file.

## Functions

### Basic Declaration

```myco
func greet(name):
    print("Hello, " .. name)
end
```

### With Return Type

```myco
func add(x: int, y: int) -> int:
    return x + y
end
```

### Return Type First

```myco
int add(x: int, y: int):
    return x + y
end
```

### Inferred Parameters

```myco
func log(msg):
    print(msg)
end
```

### Multiple Return Values

```myco
func minmax(nums: list) -> int, int:
    return min(nums), max(nums)
end
```

### Lambdas

```myco
let square = (x) => x * x
```

## Loops

### While Loop

```myco
let i = 0
while i < 10:
    print(i)
    i++
end
```

### Conditional End Loop

```myco
let i = 0
while true:
    print(i)
    i++
end when i == 10
```

### For-Each Loop

```myco
for item in list:
    print(item)
end
```

### Classic For Loop (C-style)

```myco
for let i = 0; i++; i < 10:
    print(i)
end
```

* Supports scoped variables and post-increments.

## Conditionals

### If-Else

```myco
if x > 5:
    print("High")
elseif x == 5:
    print("Equal")
else:
    print("Low")
end
```

### Switch-Case

```myco
switch fruit:
    case "apple":
        print("Red")
    case "banana":
        print("Yellow")
    else:
        print("Unknown")
end
```

## Exception Handling

```myco
try:
    let content = read_file("data.txt")
catch err:
    print("Error: " .. err)
end
```

## File I/O

```myco
write_file("log.txt", "Hello!")
let lines = read_lines("file.txt")
```

## Built-in Functions

* `print()`, `len()`, `str()`, `int()`, `float()`
* `read_file()`, `write_file()`, `read_lines()`
* `random()`, `min()`, `max()`
* String: `split()`, `upper()`, `lower()`, `replace()`, `trim()`
* List: `append()`, `insert()`, `remove()`

## Nullable Types and None

```myco
let ghost = none
if ghost == none:
    print("No value")
```

## Type Conversion

```myco
str(123)
int("42")
float("3.14")
```

## Memory Management

* Myco uses automatic garbage collection for simplicity and speed.
* Manual memory management is not exposed.

## Concurrency and Async

Myco supports async syntax with cooperative scheduling determined at compile-time.

```myco
async wait(ms: int):
    print("Waiting " .. ms)
    wait(ms)
end
```

## Comments and Docstrings

```myco
# This is a comment
"""This is a docstring"""
```

## All-in-One Example Script

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

## Myco Overview

Myco is a compiled programming language designed for speed, simplicity, and minimal binary size. It’s ideal for developers who need expressive, readable code that compiles into compact, fast executables.

### Key Features:

* **Very Small Size**: Compiled programs are usually between 50–150 KB.
* **Extremely Fast Compilation**: Myco compiles source files nearly instantly.
* **Windowed and CLI Support**: Myco can target graphical apps and command-line tools alike.
* **Hot Reloading**: Changes can be reflected during runtime for development.
* **Distributable**: A single Myco executable (`myco.exe` on Windows, `myco` on macOS/Linux) can be used to run `.myco` files instantly.
* **Human Readable, Machine Fast**: Easy to write, powerful to use.

### Use Cases:

* Embedded utilities
* Tools and scripts
* Educational software
* Lightweight games
* Modular desktop utilities

Myco helps developers build and distribute high-performance applications with minimal complexity, while maintaining expressiveness and robust functionality.
