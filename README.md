# 🍄 Myco Language

> A whimsical, mystical programming language rooted deep in the forest floor.  
> 🌿 Think C meets Lua — but with mushrooms.

Discord: https://discord.gg/CR8xcKb3zM

**Current Version**: v1.4.0 - Float Support & File I/O

---

## 🌟 What is Myco?

**Myco** is a lightweight, expressive scripting language designed for simplicity, readability, and just a touch of magic. Inspired by every aspect of other languages I hate and my weird obsession with Fungi, Myco is built to be both intuitive and powerful for small scripts or full programs. I did not use AI to code this project, I do not like using AI for my work as it is buggy and does not give me good results.

---

## 🍃 Features

- 🌙 **Clean, readable syntax**
- 🔮 **Dynamic typing** with `let`
- 🍂 **Functions, control flow, and loops**
- 🌲 **List and map support** ✅ **Complete**
- 🧪 **Try-catch error handling** ✅ **v1.3.3**
- 🧙 **Mystical and themed syntax highlighting** with the Myco VS Code Extension
- 🧮 **Math library** with 15+ mathematical functions and float support ✅ **v1.4.0**
- ➡️ **Arrow syntax** for function return types ✅ **v1.3.1**
- 🎯 **Utility library** with 20+ functions ✅ **v1.3.2**
- 🔍 **Clear type system** (no more cryptic codes) ✅ **v1.3.2**
- 🔀 **Switch/case statements** ✅ **v1.3.3**
- 🎯 **Lambda functions** with functional programming support
- 🔄 **Implicit functions** with optional type annotations
- 🌊 **Floating-point numbers** with full arithmetic support ✅ **v1.4.0**

---

## 🔧 Example

```myco
let name = "Ivy";

# Traditional syntax
func greet(person):
    print("Welcome to the woods,", person, "!");
end

# New arrow syntax
func calculate_power(base: int, exponent: int) -> int:
    return pow(base, exponent);
end

greet(name);
let result = calculate_power(2, 8);  # 256

# Utility library examples
let user = {name: "Alice", age: 25};
debug(user);                    # Detailed object inspection
let user_type = type(user);     # Returns "Object"
let has_name = has(user, "name"); # Returns 1 (true)

for item in [1, 2, 3]:
    print("Found a mushroom with ID:", item);
end

# Switch statement example
let day = 3;
switch day:
    case 1:
        print("Monday");
    case 2:
        print("Tuesday");
    case 3:
        print("Wednesday");
    default:
        print("Weekend");
end

# Try-catch error handling
try:
    let result = 10 / 0;
catch error:
    print("Caught error:", error);
end

# Float arithmetic examples
let pi = 3.14159;
let radius = 5.5;
let area = pi * pow(radius, 2);  # Area of circle
let circumference = 2 * pi * radius;

# Mixed integer and float operations
let mixed = 10 + 3.14;           # 13.14
let scaled = 5 * 2.5;            # 12.5
```
