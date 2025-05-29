# 🍄 Myco Language Documentation

Welcome to the official documentation for **Myco**, the mystical mushroom-themed scripting language. This document serves as a comprehensive reference for Myco’s syntax, features, and design philosophy.

---

## 🌿 Table of Contents

1. [Introduction](#introduction)
2. [Basic Syntax](#basic-syntax)
3. [Variables](#variables)
4. [Data Types](#data-types)
5. [Operators](#operators)
6. [Control Flow](#control-flow)
7. [Functions](#functions)
8. [Collections](#collections)
9. [Error Handling](#error-handling)
10. [Comments](#comments)
11. [Advanced Features](#advanced-features)
12. [Best Practices](#best-practices)
13. [Tooling & Extensions](#tooling--extensions)
14. [Examples](#examples)

---

## 🌱 Introduction

Myco is designed to be readable, expressive, and fun. It takes cues from Lua, Python, and functional languages while adding its own twist with a forest-themed aesthetic. It’s perfect for beginners and pros alike.

---

## 🍁 Basic Syntax

```myco
# Single-line comment
let x = 10

# Block-like structure
if x > 5:
    print("x is greater than 5")
end
```

* Indentation is optional but recommended.
* Blocks are delimited with `:` and `end`.
* You can separate multiple statements on a single line with semicolons:

  ```myco
  let a = 1; let b = 2; print(a + b)
  ```

---

## 🌾 Variables

```myco
let a = 5
var b = 10
const c = 15
```

* `let` is for regular mutable bindings.
* `var` is more explicit for mutable vars.
* `const` defines immutable values.
* Multi-variable assignments:

  ```myco
  let a, b, c = 1, 2, 3
  let (x, y, z) = (10, 5, 2)
  let name = "Ivy"; let age = 15; let city = "New York"
  ```

---

## 🍂 Data Types

* **Numbers**: `let n = 42`
* **Strings**: `let s = "hello"`
* **Booleans**: `true`, `false`
* **Lists**: `let l = [1, 2, 3]`
* **Maps**: `let m = {"key": "value"}`
* **Null/None**: `nil`

---

## 🌙 Operators

### Arithmetic

```myco
+, -, *, /, %, ^  # addition, subtraction, multiplication, division, modulo, power
```

### Logical

```myco
and, or, not
```

### Comparison

```myco
==, !=, <, >, <=, >=
```

### String Concatenation

```myco
"Hello" .. "World"  # => HelloWorld
```

---

## 🍃 Control Flow

### If-Else

```myco
if condition:
    ...
elseif other:
    ...
else:
    ...
end
```

### While

```myco
while condition:
    ...
end
```

### For Loops

```myco
for i = 0; i < 5; i = i + 1:
    print(i)
end

for item in [1, 2, 3]:
    print(item)
end

for i in 1:10:
    if i % 2 == 0:
        print("Even:", i);
    else:
        print("Odd:", i);
    end
end
```

### Switch

```myco
switch result:
    case 120:
        print("Correct!");
    default:
        print("Wrong!");
end
```

---

## 🔮 Functions

```myco
func greet(name):
    print("Hello, " .. name)
end

greet("Ivy")
```

* Supports recursion, default parameters (planned), and closures (planned).
* Functions with type signatures:

  ```myco
  func square(x: number) -> number:
      return x * x
  end
  ```
* More examples:

  ```myco
  func foo() -> string:
      print("void function")
  end

  func bar() -> int:
      return 42;
  end
  ```

  ```myco
  func factorial(n: int):
      if n <= 1:
          return 1;
      end
      return n * factorial(n - 1);
  end
  ```

---

## 🍄 Collections

### Lists

```myco
let fruits = ["apple", "banana"]
fruits[0]  # "apple"
```

### Maps

```myco
let obj = {"name": "Myco", "type": "lang"}
print(obj["name"])
```

---

## 🌧️ Error Handling

```myco
try:
    let result = 1 / 0
catch e:
    print("Caught error:", e)
end

try:
    let x = 10 / 0;
catch err:
    print("Error:", err);
end
```

* Catch block variable `e` is optional.

---

## 🍃 Comments

```myco
# This is a single-line comment
let x = 10  # Inline comment
```

---

## 🎐 Advanced Features (planned)

* Modules and Imports
* Pattern Matching
* Coroutines / async
* File I/O
* Type Hints (gradual typing)
* Meta-programming (macros)

---

## 🌲 Best Practices

* Use descriptive names: `let user_age` over `let a`
* Keep indentation consistent
* Limit complex expressions per line
* Use `const` for immutable values
* Prefer pure functions when possible

---

## 👩‍🔭 Tooling & Extensions

* **VS Code Extension**: Syntax highlighting, autocomplete, forest-themed UI
* **CLI Interpreter (planned)**: `myco run script.myco`
* **File Association**: Use Platypus on macOS to bundle `.myco` with a custom icon

---

## 🧪 Examples

```myco
let x = 10
let y = 20
func add(a, b):
    return a + b
end
print("Sum:", add(x, y))
```

```myco
let animals = ["deer", "fox", "owl"]
for a in animals:
    print("Seen a", a)
end
```

```myco
switch weather:
case "rain":
    print("Bring a cloak")
case "sun":
    print("Wear a hat")
default:
    print("Who knows!")
end
```

```myco
let str1 = "Hello, "; let str2 = "World"; let concat = str1 .. str2; print(concat);
```

---

## 🪵 Final Notes

Myco is a living language — evolving like a fungal network. This doc will grow alongside it.

🌸 Made by Ivy, with moss and love.
