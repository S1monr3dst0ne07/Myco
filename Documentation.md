# Myco Programming Language Documentation

## Table of Contents
1. [Overview](#overview)
2. [Installation](#installation)
3. [Basic Syntax](#basic-syntax)
4. [Data Types](#data-types)
5. [Variables](#variables)
6. [Operators](#operators)
7. [Objects](#objects)
8. [Arrays](#arrays)
9. [Sets](#sets)
10. [Control Flow](#control-flow)
11. [Functions](#functions)
12. [Built-in Functions](#built-in-functions)
13. [Standard Library](#standard-library)
14. [Examples](#examples)
15. [Error Handling](#error-handling)

## Overview

Myco is a modern, lightweight programming language designed for simplicity and expressiveness. It features dynamic typing, object-oriented capabilities, functional programming with lambda functions, and a clean syntax inspired by Lua and Python.

**Version**: 1.4.0 - Float Support & File I/O  
**License**: MIT  
**Repository**: https://github.com/IvyMycelia/myco

**Latest Features**: Comprehensive float support, enhanced math functions, clear type system, comprehensive utility library, arrow syntax for functions, switch/case statements, try/catch error handling

## Installation

### Prerequisites
- GCC compiler
- Make build system
- Git (for source installation)

### Building from Source
```bash
git clone https://github.com/TrendyBananaYT/myco.git
cd myco/myco
make
```

### Running Programs
```bash
./myco filename.myco
```

## Basic Syntax

### Comments
```myco
# Single-line comment
```

### Statements
All statements end with a semicolon:
```myco
print("Hello, World!");
let x = 42;
```

### Printing
```myco
print("Hello");
print("Value:", 42);
print(variable);
```

## Data Types

### Numbers
```myco
let integer = 42;
let negative = -17;
let zero = 0;
let pi = 3.14159;
let small = 0.001;
let negative_float = -2.5;
let leading_decimal = .25;
```

### Floating-Point Numbers ⭐ **NEW in v1.4.0**
Myco now supports floating-point numbers with full arithmetic operations:

```myco
# Float literals
let pi = 3.14159;
let e = 2.71828;
let small = 0.001;
let large = 1000.0;

# Leading decimal point
let quarter = .25;
let half = .5;

# Negative floats
let negative_pi = -3.14159;
let negative_small = -0.001;

# Float arithmetic
let sum = 3.14 + 2.5;        # 5.64
let product = 3.14 * 2.0;    # 6.28
let quotient = 6.28 / 2.0;   # 3.14
let difference = 3.14 - 2.5; # 0.64

# Mixed integer and float
let mixed = 10 + 3.14;       # 13.14
let scaled = 5 * 2.5;        # 12.5
```

### Type System ⭐ **NEW in v1.3.2**
Myco features a clear, descriptive type system that returns meaningful names instead of cryptic codes:

```myco
# Type checking with clear names
let num_type = type(42);        # Returns: "Integer"
let str_type = type("hello");   # Returns: "String"
let arr_type = type([1,2,3]);  # Returns: "Array"
let obj_type = type({x:1});    # Returns: "Object"

# Boolean type checks
let is_number = is_num(42);     # Returns: 1 (True)
let is_string = is_str("test"); # Returns: 1 (True)
let is_array = is_arr([1,2]);   # Returns: 1 (True)
let is_object = is_obj({x:1});  # Returns: 1 (True)
```

### Strings
```myco
let name = "Alice";
let message = "Hello, World!";
let empty = "";
```

### Booleans
```myco
# Boolean keywords (recommended)
let isTrue = True;     # True
let isFalse = False;   # False

# Numeric booleans (legacy support)
let isValid = 1;       # True (1)
let isInvalid = 0;     # False (0)
```

### Arrays
```myco
let numbers = [1, 2, 3, 4, 5];
let strings = ["apple", "banana", "cherry"];
let mixed = [1, "hello", 42];
```

### Objects
```myco
let person = {
    name: "Alice",
    age: 30,
    city: "New York"
};
```

### Sets
Sets are collections of unique elements. Currently, Sets are created and manipulated using built-in functions.
```myco
# Sets are created programmatically using set functions
# Note: Set literal syntax {1, 2, 3} coming in future version
```

## Variables

### Declaration and Assignment
```myco
let variableName = value;
```

### Examples
```myco
let name = "Bob";
let age = 25;
let isActive = True;
let scores = [95, 87, 92];
```

### Variable Naming Rules
- Must start with a letter (a-z, A-Z)
- Can contain letters, numbers, and underscores
- Case-sensitive
- Cannot use reserved keywords

## Operators

### Arithmetic Operators
```myco
let a = 10;
let b = 3;

let sum = a + b;        # Addition: 13
let diff = a - b;       # Subtraction: 7
let product = a * b;    # Multiplication: 30
let quotient = a / b;   # Division: 3
let remainder = a % b;  # Modulo: 1
```

### String Concatenation
```myco
let firstName = "John";
let lastName = "Doe";
let fullName = firstName + " " + lastName;  # "John Doe"
```

### Comparison Operators
```myco
let x = 10;
let y = 20;

let equal = x == y;         # Equality
let notEqual = x != y;      # Inequality
let less = x < y;           # Less than
let greater = x > y;        # Greater than
let lessEqual = x <= y;     # Less than or equal
let greaterEqual = x >= y;  # Greater than or equal
```

### Logical Operators
```myco
let a = 1;  # True
let b = 0;  # False

let and = a && b;   # Logical AND
let or = a || b;    # Logical OR
let not = !a;       # Logical NOT
```

## Objects

### Object Creation
```myco
let person = {
    name: "Alice",
    age: 30,
    email: "alice@example.com"
};
```

### Property Access

#### Dot Notation
```myco
let name = person.name;
let age = person.age;
```

#### Bracket Notation
```myco
let name = person["name"];
let age = person["age"];

# Dynamic property access
let property = "name";
let value = person[property];
```

### Property Assignment

#### Dot Notation Assignment
```myco
person.name = "Bob";
person.age = 25;
```

#### Bracket Notation Assignment
```myco
# String literal keys
person["email"] = "bob@example.com";
person["phone"] = "123-456-7890";

# Variable keys (dynamic assignment)
let propertyName = "address";
person[propertyName] = "123 Main St";

# Numeric values
person["score"] = 95;
person["rank"] = 1;
```

### Nested Objects
```myco
let user = {
    name: "Alice",
    address: {
        street: "123 Main St",
        city: "New York",
        zip: "10001"
    }
};

let city = user.address.city;
let street = user["address"]["street"];
```

## Arrays

### Array Creation
```myco
let numbers = [1, 2, 3, 4, 5];
let fruits = ["apple", "banana", "orange"];
let empty = [];
```

### Array Access
```myco
let first = numbers[0];    # First element
let second = numbers[1];   # Second element
let last = numbers[4];     # Last element
```

### Array Assignment
```myco
numbers[0] = 10;
fruits[1] = "grape";
```

### Functional Array Operations ⭐ **NEW in v1.2.1**
Myco now supports functional programming operations on arrays using lambda functions:

#### Filtering Arrays
```myco
let numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

# Filter even numbers using lambda
let isEven = x => x % 2 == 0;
let evens = filter(numbers, isEven);

# Filter long words
let words = ["hello", "world", "myco", "lambda"];
let longWords = filter(words, word => len(word) > 4);
```

#### Transforming Arrays
```myco
# Double all numbers
let double = x => x * 2;
let doubled = map(numbers, double);

# Get word lengths
let wordLengths = map(words, word => len(word));
```

#### Reducing Arrays
```myco
# Sum all numbers
let add = (acc, x) => acc + x;
let sum = reduce(numbers, 0, add);

# Find maximum value
let max = reduce(numbers, numbers[0], (acc, x) => acc > x ? acc : x);
```

**Note**: These functions maintain backward compatibility with numeric operation codes while adding powerful lambda support.

## Sets

Sets are collections that automatically maintain unique elements. Myco's Set data type provides efficient operations for mathematical set operations and data deduplication.

### Set Creation
Currently, Sets are created programmatically using the `create_set()` function and manipulated with built-in Set functions:
```myco
# Sets are created and managed using built-in functions
# Future versions will support Set literal syntax: {1, 2, 3}
```

### Set Operations
Sets automatically ensure all elements are unique and provide efficient membership testing.

#### Adding Elements
```myco
# Note: Sets must be created programmatically in current version
# set_add(setName, element) - adds element if not already present
```

#### Checking Membership
```myco
# set_has(setName, element) - returns 1 if element exists, 0 otherwise
```

#### Getting Set Size
```myco
# set_size(setName) - returns number of unique elements in the set
```

### Set Characteristics
- **Unique Elements**: Automatically prevents duplicate values
- **Efficient Lookup**: Fast membership testing
- **Type Support**: Supports both numeric and string sets
- **Memory Managed**: Automatic allocation and cleanup

### Future Enhancements
Upcoming versions will include:
- Set literal syntax: `{1, 2, 3}`
- Set operations: `union()`, `intersection()`, `difference()`
- Set iteration and advanced manipulation

## Control Flow

### Conditional Statements

#### If Statement
```myco
if condition:
    # code block
end
```

#### If-Else Statement
```myco
if condition:
    # code if true
else:
    # code if false
end

# Using boolean keywords
if True:
    print("This always executes");
end

if False:
    print("This never executes");
else:
    print("This executes instead");
end

# Boolean variables
let user_authenticated = True;
let has_permission = False;

if user_authenticated and has_permission:
    print("Access granted");
else:
    print("Access denied");
end
```

#### If-Else If Statement
```myco
if condition1:
    # code block 1
else if condition2:
    # code block 2
else:
    # default code block
end
```

### Loops

#### While Loop
```myco
while condition:
    # code block
end
```

#### For Loop
```myco
for i in start:end:
    # code block
end
```

### Switch Statement ⭐ **NEW in v1.3.2**
```myco
switch expression:
    case value1:
        # code block for value1
    case value2:
        # code block for value2
    default:
        # default code block
end
```

**Features:**
- **Expression-based switching**: Evaluate any expression for case matching
- **Multiple cases**: Support for multiple case values
- **Default case**: Optional default case for unmatched values
- **Fall-through behavior**: Cases execute until break or end
- **Type flexibility**: Works with integers, strings, and expressions

**Example:**
```myco
let day = 3;
switch day:
    case 1:
        print("Monday");
    case 2:
        print("Tuesday");
    case 3:
        print("Wednesday");
    case 4:
        print("Thursday");
    case 5:
        print("Friday");
    default:
        print("Weekend");
end
```

### Try-Catch Error Handling ⭐ **NEW in v1.3.2**
```myco
try:
    # code that might cause an error
catch errorVariable:
    # code to handle the error
end
```

**Features:**
- **Error catching**: Automatically catches runtime errors
- **Error variable**: Access error information in catch block
- **Graceful handling**: Program continues after error handling
- **Built-in error types**: Handles division by zero, invalid operations, etc.

**Example:**
```myco
try:
    let result = 10 / 0;  # This will cause an error
    print("This won't print");
catch error:
    print("Caught error:", error);
    print("Continuing execution...");
end

# Program continues here
print("After error handling");
```

### Examples
```myco
let count = 0;
while count < 5:
    print("Count:", count);
    count = count + 1;
end

for i in 0:10:
    print("Number:", i);
end
```

## Functions

### Function Declaration
Myco supports two syntaxes for function declarations:

#### Traditional Colon Syntax
```myco
func functionName(parameter1, parameter2):
    # function body
    return value;
end
```

#### New Arrow Syntax ⭐ **NEW in v1.3.1**
```myco
func functionName(parameter1, parameter2) -> returnType:
    # function body
    return value;
end
```

#### Type Annotations
```myco
# With parameter types
func add(a: int, b: int) -> int:
    return a + b;
end

# With mixed types
func process(x: int, y) -> string:
    return "Result: " + to_string(x + y);
end

# No types (implicit)
func multiply(a, b):
    return a * b;
end
```

### Function Call
```myco
let result = functionName(argument1, argument2);
```

### Lambda Functions ⭐ **NEW in v1.2.1**
Myco now supports lambda functions (arrow functions) for functional programming:

#### Basic Lambda Syntax
```myco
# Single parameter
let double = x => x * 2;
let result = double(5);  # 10

# Multiple parameters
let add = (a, b) => a + b;
let sum = add(10, 20);  # 30

# No parameters
let getFive = () => 5;
let value = getFive();  # 5
```

#### Lambda with Variable Capture
```myco
let base = 10;
let addBase = x => x + base;  # Captures 'base' variable
let result = addBase(5);      # 15
```

#### Complex Lambda Expressions
```myco
let square = x => x * x;
let isEven = x => x % 2 == 0;
let complex = x => square(x) + x + 1;

let result = complex(3);  # 13
```

#### Nested Lambda Calls
```myco
let double = x => x * 2;
let add = (a, b) => a + b;
let result = add(double(3), double(4));  # 14
```

### Implicit Functions ⭐ **NEW in v1.2.4**
Myco now supports **true implicit functions** in the Python style - functions that don't require explicit type annotations and can return values implicitly.

#### Optional Type Annotations
```myco
# No parameter type annotations needed
func add_numbers(a, b):           # Types are inferred automatically
    return a + b;
end

# Mixed type annotations supported
func mixed_types(x, y: int):      # x is implicit, y is explicit
    return x + y;
end

# No return type annotation needed
func multiply(a, b):              # Return type is inferred
    return a * b;
end
```

#### Implicit Returns
```myco
# Functions can return nothing implicitly
func process_data(x):
    print("Processing:", x);
    # No return statement = implicit return of 0
end

# Conditional returns work naturally
func flexible_return(x):
    if x > 0:
        return x * 2;             # Returns number
    # No return for x <= 0 = implicit return of 0
end
```

#### Complex Logic
```myco
# Recursive functions with implicit types
func factorial(n):
    if n <= 1:
        return 1;
    else:
        return n * factorial(n - 1);
    end
end

# Multiple return paths
func conditional_logic(x, y):
    let temp = x + y;
    if temp > 100:
        return temp / 2;
    else:
        if temp < 0:
            return temp * -1;
        else:
            return temp;
        end
    end
end
```

#### Benefits
- **Python-like Syntax**: Write functions without verbose type annotations
- **Flexible Returns**: Functions can return different types or nothing
- **Cleaner Code**: Less boilerplate, more readable functions
- **Backward Compatible**: Existing typed functions continue to work
- **Developer Friendly**: Faster prototyping and development

### Operator Overloading (Also Available)
Myco also supports operator overloading where operators automatically call appropriate functions:

#### Mathematical Operators
```myco
# Addition automatically calls add()
let sum = 5 + 3;        # Calls add(5, 3)

# Subtraction automatically calls subtract()
let diff = 10 - 4;      # Calls subtract(10, 4)

# Multiplication automatically calls multiply()
let product = 7 * 6;    # Calls multiply(7, 6)

# Division automatically calls divide()
let quotient = 15 / 3;  # Calls divide(15, 3)

# Modulo automatically calls modulo()
let remainder = 17 % 3; # Calls modulo(17, 3)
```

#### Comparison & Logical Operators
```myco
# Equality automatically calls equals()
let isEqual = 5 == 5;   # Calls equals(5, 5)

# Logical AND automatically calls logical_and()
let both = (a > 0) and (b > 0);  # Calls logical_and(a > 0, b > 0)
```

### Examples
```myco
# True implicit functions
func add(a, b):
    return a + b;
end

func process(x):
    if x > 0:
        return x * 2;
    # Implicit return of 0
end

# Operator overloading
let sum = 5 + 3;        # Calls add(5, 3)
let result = add(10, 20);
```

## Built-in Functions

### Array Functions

#### `len(array)`
Returns the length of an array or string.
```myco
let numbers = [1, 2, 3, 4];
let length = len(numbers);  # 4

let text = "Hello";
let textLength = len(text); # 5
```

#### `push(array, element)`
Adds an element to the end of an array.
```myco
let fruits = ["apple", "banana"];
push(fruits, "orange");
# fruits is now ["apple", "banana", "orange"]
```

#### `pop(array)`
Removes and returns the last element of an array.
```myco
let numbers = [1, 2, 3];
let last = pop(numbers);  # 3
# numbers is now [1, 2]
```

#### `first(array)`
Returns the first element of an array.
```myco
let numbers = [10, 20, 30];
let firstNum = first(numbers);  # 10
```

#### `last(array)`
Returns the last element of an array.
```myco
let numbers = [10, 20, 30];
let lastNum = last(numbers);  # 30
```

#### `reverse(array)`
Reverses the order of elements in an array.
```myco
let numbers = [1, 2, 3, 4];
reverse(numbers);
# numbers is now [4, 3, 2, 1]
```

#### `slice(array, start, end)`
Returns a portion of an array from start to end (exclusive).
```myco
let numbers = [1, 2, 3, 4, 5];
let subset = slice(numbers, 1, 4);  # [2, 3, 4]
```

#### `join(array, separator)`
Joins array elements into a string with a separator.
```myco
let words = ["Hello", "World"];
let sentence = join(words, " ");  # "Hello World"
```

#### `filter(array, condition)` ⭐ **NEW: Lambda Support**
Filters array elements based on a condition or lambda function.
```myco
# Numeric operation codes (legacy)
let evens = filter(numbers, 1);  # Filter even numbers

# Lambda functions (v1.2.1+)
let isEven = x => x % 2 == 0;
let evens = filter(numbers, isEven);

let longWords = filter(words, word => len(word) > 4);
```

#### `map(array, operation)` ⭐ **NEW: Lambda Support**
Transforms array elements using an operation or lambda function.
```myco
# Numeric operation codes (legacy)
let doubled = map(numbers, 0);  # Double all numbers

# Lambda functions (v1.2.1+)
let double = x => x * 2;
let doubled = map(numbers, double);

let wordLengths = map(words, word => len(word));
```

#### `reduce(array, operation, initial)` ⭐ **NEW: Lambda Support**
Reduces array to a single value using an operation or lambda function.
```myco
# Numeric operation codes (legacy)
let sum = reduce(numbers, 0, 0);  # Sum all numbers

# Lambda functions (v1.2.1+)
let add = (acc, x) => acc + x;
let sum = reduce(numbers, 0, add);

let product = reduce(numbers, 1, (acc, x) => acc * x);
```

### String Functions

#### `split(string, delimiter)`
Splits a string into an array using a delimiter.
```myco
let text = "apple,banana,cherry";
let fruits = split(text, ",");  # ["apple", "banana", "cherry"]
```

#### `trim(string)`
Removes whitespace from the beginning and end of a string.
```myco
let text = "  Hello World  ";
let cleaned = trim(text);  # "Hello World"
```

#### `replace(string, old, new)`
Replaces all occurrences of a substring with another string.
```myco
let text = "Hello World";
let newText = replace(text, "World", "Universe");  # "Hello Universe"
```

### Object Functions

#### `object_keys(object)`
Returns an array of object property names.
```myco
let person = {name: "Alice", age: 30};
let keys = object_keys(person);  # ["name", "age"]
```

#### `values(object)`
Returns an array of object property values.
```myco
let person = {name: "Alice", age: 30};
let vals = values(person);  # ["Alice", "30"]
```

#### `has_key(object, key)`
Checks if an object has a specific property.
```myco
let person = {name: "Alice", age: 30};
let hasName = has_key(person, "name");    # 1 (true)
let hasEmail = has_key(person, "email");  # 0 (false)
```

#### `size(object)`
Returns the number of properties in an object.
```myco
let person = {name: "Alice", age: 30, city: "NYC"};
let count = size(person);  # 3
```

#### `remove(object, key)`
Removes a property from an object.
```myco
let person = {name: "Alice", age: 30, temp: "delete_me"};
let success = remove(person, "temp");  # 1 (true)
print("Person:", person);  # {name: "Alice", age: 30}

# Variable key
let key = "age";
let result = remove(person, key);  # 1 (true)
print("Person:", person);  # {name: "Alice"}

# Non-existent property
let failed = remove(person, "missing");  # 0 (false)
```

### Utility Functions

#### `to_string(value)`
Converts a value to its string representation.
```myco
let number = 42;
let text = to_string(number);  # "42"
```

### Set Functions

#### `set_has(set, element)`
Checks if a set contains a specific element.
```myco
# Returns 1 (true) if element exists, 0 (false) otherwise
# Supports both numeric and string elements
```

#### `set_add(set, element)`
Adds an element to a set (only if not already present).
```myco
# Returns 1 on success (including if element already exists)
# Automatically maintains uniqueness constraint
```

### Math Library Functions ⭐ **NEW in v1.4.0 - Enhanced with Float Support**

#### Mathematical Constants
```myco
let pi = PI();        # 3.141592653589793 (returned as 3141593)
let e = E();          # 2.718281828459045 (returned as 2718282)
let infinity = INF(); # 999999999 (represents infinity)
let not_a_number = NAN(); # -999999999 (represents NaN)
```

**Float Support**: All math functions now work with both integers and floating-point numbers! 🎉

#### Basic Mathematical Functions

##### `abs(number)`
Returns the absolute value of a number. Works with both integers and floats.
```myco
let result1 = abs(42);      # 42
let result2 = abs(-42);     # 42
let result3 = abs(0);       # 0
let result4 = abs(-3.14);   # 3.14
let result5 = abs(0.001);   # 0.001
```

##### `pow(base, exponent)`
Raises a number to a power. Works with both integers and floats.
```myco
let result1 = pow(2, 3);      # 8 (2³)
let result2 = pow(5, 2);      # 25 (5²)
let result3 = pow(10, 0);     # 1 (10⁰)
let result4 = pow(2.5, 3);    # 15.625 (2.5³)
let result5 = pow(3.14, 2);   # 9.8596 (π²)
```

##### `sqrt(number)`
Returns the square root of a number (positive numbers only). Works with both integers and floats.
```myco
let result1 = sqrt(16);      # 4
let result2 = sqrt(25);      # 5
let result3 = sqrt(100);     # 10
let result4 = sqrt(9.0);     # 3.0
let result5 = sqrt(2.25);    # 1.5
```

##### `floor(number)` and `ceil(number)`
Returns the floor or ceiling of a number (for integers, same as input).
```myco
let result1 = floor(42);  # 42
let result2 = ceil(42);   # 42
```

##### `min(numbers...)` and `max(numbers...)`
Returns the minimum or maximum of multiple numbers. Works with both integers and floats.
```myco
let min_val = min(5, 3, 8, 1, 9);           # 1
let max_val = max(5, 3, 8, 1, 9);           # 9
let float_min = min(3.14, 2.5, 4.0);        # 2.5
let float_max = max(3.14, 2.5, 4.0);        # 4.0
let mixed_min = min(5, 3.14, 2.5, 10);      # 2.5
```

#### Random Number Generation

##### `random()`
Returns a random number between 0.0 and 1.0 (scaled to integer).
```myco
let rand1 = random();  # Random value between 0 and 1000000
let rand2 = random();  # Different random value each time
```

##### `randint(min, max)`
Returns a random integer between min and max (inclusive).
```myco
let dice = randint(1, 6);      # Random number 1-6
let coin = randint(0, 1);      # Random number 0 or 1
```

##### `choice(array)`
Returns a random choice from an array (currently returns 1-100).
```myco
let fruits = ["apple", "banana", "cherry"];
let random_fruit = choice(fruits);  # Random number 1-100
```

#### Complex Mathematical Operations
```myco
# Combine multiple math functions
let complex = pow(2, 3) + sqrt(16) + abs(-5);  # 8 + 4 + 5 = 17

# Use in calculations
let area = PI() * pow(5, 2);  # Area of circle with radius 5
let hypotenuse = sqrt(pow(3, 2) + pow(4, 2));  # Pythagorean theorem
```

#### Float Mathematics Examples ⭐ **NEW in v1.4.0**
```myco
# Scientific calculations with floats
let pi = 3.14159;
let radius = 5.5;
let area_circle = pi * pow(radius, 2);  # Area of circle

# Trigonometric approximations
let angle_rad = 0.785398;  # 45 degrees in radians
let sin_45 = angle_rad - pow(angle_rad, 3) / 6 + pow(angle_rad, 5) / 120;

# Financial calculations
let principal = 1000.0;
let rate = 0.05;  # 5% annual interest
let years = 3.5;
let compound_interest = principal * pow(1 + rate, years);

# Statistical operations
let data = [3.14, 2.71, 1.41, 2.23];
let avg = (data[0] + data[1] + data[2] + data[3]) / 4;
let min_val = min(data[0], data[1], data[2], data[3]);
let max_val = max(data[0], data[1], data[2], data[3]);
```

#### `set_size(set)`
Returns the number of unique elements in a set.
```myco
# Returns integer count of elements
# Empty sets return 0
```

**Note**: Set creation and literal syntax will be enhanced in future versions. Current implementation provides the foundation for advanced Set operations.

### Utility Library Functions ⭐ **NEW in v1.3.2**

#### Debugging & Type Checking
```myco
# Enhanced debugging with type information
debug("Hello World");        # DEBUG: string = "Hello World" (length: 11)
debug(42);                   # DEBUG: number = 42
debug([1, 2, 3]);           # DEBUG: array (use len() to get size)

# Type checking functions
let num_type = type(42);     # "Integer"
let str_type = type("test"); # "String"
let arr_type = type([1,2]); # "Array"

# Boolean type checks
let is_number = is_num(42);  # 1 (true)
let is_string = is_str("test"); # 1 (true)
let is_array = is_arr([1,2]);   # 1 (true)
let is_object = is_obj({x: 1}); # 1 (true)
```

#### String Utilities
```myco
# String conversion alias
let str_value = str(42);     # Same as to_string(42)

# Substring search
let pos = find("Hello World", "World");  # 6 (position of "World")
let not_found = find("Hello", "Python"); # -1 (not found)
```

#### Data Utilities
```myco
# Data copying
let original = 42;
let copy_value = copy(original);  # 42 (shallow copy)

# Object property checking
let user = {name: "Alice", age: 25};
let has_name = has(user, "name");      # 1 (true)
let has_email = has(user, "email");    # 0 (false)
```

#### Benefits
- **Clean Function Names**: Easy to remember and use (`debug`, `type`, `is_num`, etc.)
- **Enhanced Debugging**: Better development experience with detailed type information
- **Type Safety**: Comprehensive type checking and introspection
- **String Operations**: Efficient substring search and string utilities
- **Data Manipulation**: Object property checking and data copying
- **Developer Friendly**: All utilities designed for productive development

## Library Import System ⭐ **NEW in v1.4.0**

Myco now features a comprehensive library import system that provides namespace protection and organized access to built-in functionality:

### Import Syntax
```myco
use math as m;      # Import math library with alias 'm'
use util as u;      # Import utility library with alias 'u'
use core as c;      # Import core library with alias 'c'
```

### Available Libraries

#### Math Library (`math`)
**Constants:**
- `m.PI` - Mathematical constant π (3.14159)
- `m.E` - Euler's number e (2.71828)
- `m.INF` - Infinity
- `m.NAN` - Not a Number

**Functions:**
- `m.abs(x)` - Absolute value
- `m.pow(base, exponent)` - Power function
- `m.sqrt(x)` - Square root
- `m.min(a, b)` - Minimum of two values
- `m.max(a, b)` - Maximum of two values
- `m.random()` - Random number between 0 and 1
- `m.randint(min, max)` - Random integer in range

#### Utility Library (`util`)
**Functions:**
- `u.debug(value)` - Detailed value inspection
- `u.type(value)` - Type information
- `u.is_num(value)` - Check if value is numeric
- `u.is_str(value)` - Check if value is string
- `u.is_arr(value)` - Check if value is array
- `u.is_obj(value)` - Check if value is object
- `u.str(value)` - Convert to string
- `u.find(haystack, needle)` - Find substring
- `u.copy(value)` - Deep copy
- `u.has(object, key)` - Check object property

#### Core Library (`core`)
**Functions:**
- `c.print(...)` - Print function
- `c.len(value)` - Length function

#### File I/O Library (`file_io`) ⭐ **NEW in v1.4.0**
**Functions:**
- `f.read_file(filename)` - Read file contents, returns `1` on success, `0` on failure
- `f.write_file(filename, content)` - Write content to file, returns `1` on success, `0` on failure
- `f.list_dir(path)` - List directory contents, returns number of entries found
- `f.exists(path)` - Check if file/directory exists, returns `1` if exists, `0` if not

**Features:**
- **Large File Support**: Handles files up to 2GB in size
- **Buffer Safety**: 1024-byte filename/path buffers with overflow protection
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **Error Handling**: Comprehensive error messages for debugging
- **Memory Management**: Automatic cleanup with tracked memory allocation
- **Content Types**: Supports string literals, variables, and numeric conversion

### Benefits
- **Namespace Protection**: Built-in functions no longer pollute global namespace
- **Organized Access**: Functions grouped by functionality
- **Custom Functions**: Users can create their own functions without conflicts
- **Clear Dependencies**: Explicit imports show what functionality is used
- **Professional Structure**: Enterprise-grade organization

### Example Usage
```myco
use math as m;
use util as u;

# Math operations
let pi = m.PI;
let area = m.pow(5, 2) * pi;

# Utility functions
u.debug(area);
let area_type = u.type(area);

# Custom function (no conflict with built-ins)
func my_debug(value):
    print("Custom debug:", value);
end

my_debug(area);  # Works without conflicts

# File I/O operations
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

# Error handling example
let file_exists = f.exists("nonexistent.txt");
if file_exists == 0:
    print("File does not exist");
end

# Content type handling
let number = 42;
f.write_file("number.txt", number);  # Automatically converts to string

# Long filename support
let long_name = "this_is_a_very_long_filename_that_tests_buffer_handling.txt";
f.write_file(long_name, "Long filename test");

### Error Handling and Limitations

**Common Error Cases:**
- **Empty paths**: Returns error for empty filenames or paths
- **Invalid paths**: Clear error messages for non-existent files/directories
- **Permission issues**: Handles read/write permission errors gracefully
- **Memory allocation**: Automatic cleanup on memory allocation failures

**Limitations:**
- **Filename length**: Maximum 1023 characters (1024-byte buffer with null terminator)
- **Path length**: Maximum 1023 characters for directory paths
- **File size**: Limited by available system memory (typically 2GB+)
- **Binary files**: Content is treated as text (null bytes may cause issues)

**Best Practices:**
- Always check return values for error conditions
- Use descriptive filenames for better debugging
- Handle file operations in try-catch blocks when appropriate
- Clean up temporary files after use
```

## Standard Library

Myco includes a comprehensive standard library with functions for common programming tasks:

- **Array manipulation**: push, pop, slice, reverse, join
- **String processing**: split, trim, replace
- **Object operations**: keys, values, property checking
- **Set operations**: set_has, set_add, set_size
- **Type conversion**: to_string
- **Utility functions**: len for size checking

## Examples

### Basic Program
```myco
# Simple greeting program
let name = "Alice";
let age = 30;

print("Hello, " + name + "!");
print("You are " + to_string(age) + " years old.");
```

### Working with Arrays
```myco
# Array operations
let numbers = [1, 2, 3, 4, 5];

print("Original array:", numbers);
print("Length:", len(numbers));
print("First element:", first(numbers));
print("Last element:", last(numbers));

push(numbers, 6);
print("After push:", numbers);

let popped = pop(numbers);
print("Popped element:", popped);
print("After pop:", numbers);

reverse(numbers);
print("Reversed:", numbers);
```

### Working with Objects
```myco
# Object operations
let person = {
    name: "Bob",
    age: 25,
    city: "San Francisco",
    email: "bob@example.com"
};

print("Person:", person);
print("Name:", person.name);
print("Age:", person["age"]);

let keys = object_keys(person);
print("Properties:", keys);

let vals = values(person);
print("Values:", vals);

print("Has email?", has_key(person, "email"));
print("Has phone?", has_key(person, "phone"));
```

### String Processing
```myco
# String manipulation
let text = "  apple,banana,cherry  ";

let cleaned = trim(text);
print("Trimmed:", cleaned);

let fruits = split(cleaned, ",");
print("Split:", fruits);

let joined = join(fruits, " | ");
print("Joined:", joined);

let replaced = replace(joined, "banana", "grape");
print("Replaced:", replaced);
```

### Control Flow Example
```myco
# Conditional logic and loops
let scores = [85, 92, 78, 96, 88];
let total = 0;
let count = len(scores);

# Calculate total
for i in 0:count:
    total = total + scores[i];
end

let average = total / count;
print("Average score:", average);

# Grade classification
if average >= 90:
    print("Grade: A");
else if average >= 80:
    print("Grade: B");
else if average >= 70:
    print("Grade: C");
else:
    print("Grade: F");
end
```

### Function Example
```myco
# Function definition and usage
func calculateArea(length, width):
    return length * width;
end

func displayRectangle(length, width):
    let area = calculateArea(length, width);
    print("Rectangle: " + to_string(length) + " x " + to_string(width));
    print("Area: " + to_string(area));
end

displayRectangle(5, 3);
displayRectangle(8, 4);
```

## Error Handling

### Common Errors

#### Syntax Errors
- Missing semicolons
- Unmatched braces or brackets
- Invalid identifiers

#### Runtime Errors
- Accessing undefined variables
- Array index out of bounds
- Invalid function calls

#### Type Errors
- Incompatible operations
- Invalid property access

### Best Practices

1. **Always use semicolons** to terminate statements
2. **Check array bounds** before accessing elements
3. **Validate object properties** before access
4. **Use meaningful variable names**
5. **Test functions with various inputs**
6. **Handle edge cases** in conditional logic

### Debugging Tips

1. Use `print()` statements to trace program execution
2. Check variable values at different points
3. Verify array and object contents
4. Test with simple inputs first
5. Break complex expressions into smaller parts

## Language Features Summary

**Current Version**: 1.1.9

**Completed Features**:
- Variables and basic data types (numbers, strings, booleans)
- Arithmetic and logical operators
- Objects with dot and bracket notation
- Arrays with indexing and assignment
- **Sets with unique element collections**
- Control flow (if/else, loops)
- Function definitions and calls
- **19+ built-in functions** (arrays, strings, objects, sets)
- String concatenation and manipulation
- Nested object support with deep object creation
- Dynamic property access and bracket notation assignment
- **Advanced array operations**: filter, map, reduce
- **Comprehensive error handling**: try/catch blocks
- **Memory management**: tracked allocations preventing leaks
- **Professional testing**: 100% unit test success rate

**Upcoming Features** (v2.0.0 roadmap):
- **Set literal syntax**: `{1, 2, 3}`
- **Set operations**: `union()`, `intersection()`, `difference()`
- Enhanced error handling
- Module system
- Advanced array operations (filter, map, reduce)
- Performance optimizations
- Extended standard library
- IDE integration tools

---

*For more information, visit the [official repository](https://github.com/IvyMycelia/myco) or contribute to the project.*
