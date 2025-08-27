# 🍄 Myco Language

> A whimsical, mystical programming language rooted deep in the forest floor.  
> 🌿 Think C meets Lua — but with mushrooms.

Discord: <https://discord.gg/CR8xcKb3zM>

**Current Version**: v1.6.0 - Language Maturity & Developer Experience

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
- 📚 **Library import system** with namespace protection ✅ **v1.4.0**
- ✅ **True/False keywords** with backward compatibility ✅ **v1.4.0**
- 📁 **File I/O operations** with comprehensive file management ✅ **v1.4.0**
- 🛣️ **Path utilities** for cross-platform path manipulation ✅ **v1.5.0**
- 🌍 **Environment variables** for system configuration ✅ **v1.5.0**
- 📝 **Command-line arguments** for user input processing ✅ **v1.5.0**
- ⚡ **Process execution** for shell command execution ✅ **v1.5.0**
- 📊 **Text processing** for CSV and data handling ✅ **v1.5.0**
- 🐛 **Enhanced debugging** with professional error handling ✅ **v1.5.0**
- 🎯 **Type system foundation** for enterprise-grade type safety ✅ **v1.6.0**
- ✨ **Language polish** with enhanced lambdas and string interpolation ✅ **v1.6.0**
- 🧪 **Testing framework** with professional testing and benchmarking ✅ **v1.6.0**
- 🔗 **Advanced data structures** with enterprise-grade algorithms ✅ **v1.6.0**

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

# Library import system
use math as m;
use util as u;

# Math library examples
let pi = m.PI;                  # 3.14159
let e = m.E;                    # 2.71828
let abs_val = m.abs(-5);        # 5
let power = m.pow(2, 8);        # 256

# Utility library examples
let user = {name: "Alice", age: 25};
u.debug(user);                  # Detailed object inspection
let user_type = u.type(user);   # Returns "Object"
let has_name = u.has(user, "name"); # Returns 1 (true)

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
    print("Error:", error);
end

# v1.6.0 Language Maturity Examples
use types as t;
use polish as p;
use test as test_framework;
use data as d;

# Type System Foundation
let type_info = t.typeof("Hello World");
let is_string = t.is_type("Hello", "string");
let converted = t.cast("42", "number");

# Language Polish
let enhanced_lambda = p.enhance_lambda("(x) => x * 2");
let interpolated = p.interpolate_string("Hello ${name}!", "name=Alice");

# Testing Framework
test_framework.describe("My Test Suite");
test_framework.it("should work correctly");
test_framework.assert("2 + 2 == 4", "Basic arithmetic");

# Advanced Data Structures
let linked_list = d.create_linked_list("42");
let binary_tree = d.create_binary_tree("100");
let sorted_array = d.quicksort("[5, 2, 8, 1, 9]");
    print("Caught error:", error);
end

# Boolean keywords (new in v1.4.0)
let is_active = True;
let is_finished = False;

# System Integration Libraries (new in v1.5.0)
use path_utils as p;
use env as e;
use args as a;
use process as proc;
use text_utils as t;
use debug as d;

# Path utilities
let full_path = p.join_path("home", "user", "documents");
let dir_name = p.dirname("/home/user/file.txt");  # "/home/user"
let file_name = p.basename("/home/user/file.txt"); # "file.txt"

# Environment variables
let home_dir = e.get_env("HOME");
e.set_env("MYCO_DEBUG", "1");

# Command-line arguments
let total_args = a.arg_count();
let first_arg = a.get_arg(0);

# Process execution
let current_pid = proc.get_pid();
let current_dir = proc.get_cwd();
proc.execute("ls -la");

# Text processing
t.write_lines("data.txt", "Hello World");
let lines = t.read_lines("data.txt");
t.write_csv("data.csv", "Name,Age,City");

# Enhanced debugging
d.start_timer();
d.warn("Processing data...");
let elapsed = d.end_timer();
let stats = d.get_stats();

if is_active and not is_finished:
    print("Process is running");
end

# File I/O operations (new in v1.4.0)
use file_io as f;

# Check if directory exists
if f.exists("."):
    print("Current directory exists");
end

# List directory contents
let file_count = f.list_dir(".");
print("Found", file_count, "entries");

# Write and read files
f.write_file("data.txt", "Hello from Myco!");
let read_success = f.read_file("data.txt");
if read_success:
    print("File read successfully");
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
