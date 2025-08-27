# Myco Language BNF Grammar Specification

**Version**: v1.6.0 - Language Maturity & Developer Experience   
**Last Updated**: December 2024   
**Status**: Complete and Current   

---

## Table of Contents
1. [Program Structure](#program-structure)
2. [Lexical Elements](#lexical-elements)
3. [Expressions](#expressions)
4. [Statements](#statements)
5. [Functions](#functions)
6. [Control Flow](#control-flow)
7. [Data Structures](#data-structures)
8. [Built-in Functions](#built-in-functions)
9. [Comments](#comments)

---

## Program Structure

```bnf
<program> ::= <statement_list>

<statement_list> ::= <statement>*
<statement> ::= <expression_statement> | <declaration_statement> | <control_statement> | <function_declaration> | <return_statement> | <break_statement> | <continue_statement> | <library_import_statement>

<block> ::= "end" | <statement_list> "end"
```

---

## Lexical Elements

### Identifiers
```bnf
<identifier> ::= <letter> (<letter> | <digit> | "_")*
<letter> ::= "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z" | "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
<digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
```

### Literals
```bnf
<literal> ::= <number_literal> | <string_literal> | <boolean_literal> | <array_literal> | <object_literal>

<number_literal> ::= <integer> | <float>
<integer> ::= <digit>+ | "-" <digit>+
<float> ::= <digit>+ "." <digit>* | "." <digit>+ | "-" <digit>+ "." <digit>* | "-" "." <digit>+

<string_literal> ::= '"' <string_content>* '"'
<string_content> ::= <any_char_except_quote> | <escape_sequence>
<escape_sequence> ::= "\\" <any_char>

<boolean_literal> ::= "0" | "1" | "True" | "False"  # 0/False = false, 1/True = true

<array_literal> ::= "[" <expression_list> "]"
<expression_list> ::= <expression> ("," <expression>)*

<object_literal> ::= "{" <property_list> "}"
<property_list> ::= <property> ("," <property>)*
<property> ::= <identifier> ":" <expression>
```

---

## Expressions

### Primary Expressions
```bnf
<expression> ::= <primary_expression> | <binary_expression> | <unary_expression> | <assignment_expression> | <function_call_expression>

<primary_expression> ::= <literal> | <identifier> | <parenthesized_expression> | <array_access> | <object_access>

<parenthesized_expression> ::= "(" <expression> ")"

<array_access> ::= <identifier> "[" <expression> "]"

<object_access> ::= <identifier> "." <identifier>
<library_access> ::= <identifier> "." <identifier>  # For library.function() calls
```

### Binary Expressions
```bnf
<binary_expression> ::= <expression> <binary_operator> <expression> | <expression> "?" <expression> ":" <expression>

<binary_operator> ::= "+" | "-" | "*" | "/" | "%" | "==" | "!=" | "<" | ">" | "<=" | ">=" | "and" | "or"
```

### Unary Expressions
```bnf
<unary_expression> ::= <unary_operator> <expression>

<unary_operator> ::= "-" | "!"
```

### Assignment Expressions
```bnf
<assignment_expression> ::= <identifier> "=" <expression>
```

### Function Call Expressions
```bnf
<function_call_expression> ::= <identifier> "(" <argument_list> ")"
<argument_list> ::= <expression> ("," <expression>)*
```

---

## Statements

### Expression Statements
```bnf
<expression_statement> ::= <expression> ";"
```

### Library Import Statements ⭐ **NEW in v1.4.0**

```bnf
<library_import_statement> ::= "use" <identifier> "as" <identifier> ";"
```

### Built-in Libraries ⭐ **NEW in v1.5.0**

Myco v1.5.0 provides comprehensive system integration through specialized libraries:

#### Path Utilities Library (`path_utils`)
```bnf
<path_utils_function> ::= <identifier> "." <path_function> "(" <argument_list> ")"
<path_function> ::= "join_path" | "dirname" | "basename" | "is_absolute" | "normalize_path" | "relative_path"
```

#### Environment Variables Library (`env`)
```bnf
<env_function> ::= <identifier> "." <env_function_name> "(" <argument_list> ")"
<env_function_name> ::= "get_env" | "set_env" | "list_env" | "has_env"
```

#### Command-Line Arguments Library (`args`)
```bnf
<args_function> ::= <identifier> "." <args_function_name> "(" <argument_list> ")"
<args_function_name> ::= "get_args" | "get_arg" | "arg_count" | "parse_flags"
```

#### Process Execution Library (`process`)
```bnf
<process_function> ::= <identifier> "." <process_function_name> "(" <argument_list> ")"
<process_function_name> ::= "execute" | "get_pid" | "get_cwd" | "change_dir"
```

#### Text Processing Library (`text_utils`)
```bnf
<text_utils_function> ::= <identifier> "." <text_utils_function_name> "(" <argument_list> ")"
<text_utils_function_name> ::= "read_lines" | "write_lines" | "read_csv" | "write_csv"
```

#### Enhanced Debugging Library (`debug`)
```bnf
<debug_function> ::= <identifier> "." <debug_function_name> "(" <argument_list> ")"
<debug_function_name> ::= "warn" | "error" | "assert" | "start_timer" | "end_timer" | "get_stats" | "set_debug_mode"
```

### Declaration Statements
```bnf
<declaration_statement> ::= "let" <identifier> "=" <expression> ";"
```

### Return Statements
```bnf
<return_statement> ::= "return" <expression>? ";"
```

### Break and Continue Statements
```bnf
<break_statement> ::= "break" ";"
<continue_statement> ::= "continue" ";"
```

---

## Functions

### Function Declaration
```bnf
<function_declaration> ::= "func" <identifier> "(" <parameter_list> ")" <return_type_spec>? ":" <block>

<parameter_list> ::= <parameter> ("," <parameter>)*
<parameter> ::= <identifier> ":" <type_spec> | <identifier>

<return_type_spec> ::= ":" <type_spec> | "->" <type_spec>

<type_spec> ::= "int" | "string" | "array" | "object"
```

**Note**: Myco supports both traditional colon syntax (`: int:`) and modern arrow syntax (`-> int:`) for return types.

---

## Control Flow

### If Statements
```bnf
<if_statement> ::= "if" <expression> ":" <block> <else_clause>?

<else_clause> ::= "else" ":" <block>
```

### Switch Statements ⭐ **NEW in v1.3.2**
```bnf
<switch_statement> ::= "switch" <expression> ":" <switch_cases> "end"

<switch_cases> ::= <case_statement>* <default_statement>?

<case_statement> ::= "case" <expression> ":" <case_body>

<case_body> ::= <statement>*

<default_statement> ::= "default" ":" <default_body>

<default_body> ::= <statement>*
```

### Try-Catch Statements ⭐ **NEW in v1.3.2**
```bnf
<try_catch_statement> ::= "try" ":" <try_body> "catch" <identifier> ":" <catch_body> "end"

<try_body> ::= <statement>*

<catch_body> ::= <statement>*
```

### For Loops
```bnf
<for_statement> ::= "for" <identifier> "in" <expression> ".." <expression> ":" <block>
```

### While Loops
```bnf
<while_statement> ::= "while" <expression> ":" <block>
```

---

## Data Structures

### Arrays
```bnf
<array_operation> ::= <array_access> | <array_method_call>

<array_method_call> ::= <identifier> "." <array_method> "(" <argument_list> ")"
<array_method> ::= "push" | "pop" | "length" | "get" | "set"
```

### Objects
```bnf
<object_operation> ::= <object_access> | <object_method_call>

<object_method_call> ::= <identifier> "." <object_method> "(" <argument_list> ")"
<object_method> ::= "keys" | "values" | "has" | "get" | "set"
```

---

## Built-in Functions

### Math Library (v1.3.0)
```bnf
<math_function> ::= "abs" | "pow" | "sqrt" | "floor" | "ceil" | "min" | "max" | "random" | "randint" | "choice"

<math_constant> ::= "PI" | "E" | "INF" | "NAN"
```

### Utility Library (v1.3.2)
```bnf
<utility_function> ::= "debug" | "type" | "is_num" | "is_str" | "is_arr" | "is_obj" | "str" | "find" | "copy" | "has"
```

### Core Functions
```bnf
<core_function> ::= "print" | "to_string" | "input" | "len"
```

### Function Call Grammar
```bnf
<builtin_function_call> ::= <math_function> "(" <argument_list> ")" | <utility_function> "(" <argument_list> ")" | <core_function> "(" <argument_list> ")"
```

---

## Comments

### Single-line Comments
```bnf
<single_line_comment> ::= "#" <any_char_except_newline>*
```

### Multi-line Comments
```bnf
<multi_line_comment> ::= "'''" <any_char>* "'''"
```

---

## Complete Program Example

```bnf
<complete_program> ::= <comment>* <statement_list>

# Example Myco program structure:
# 1. Comments (optional)
# 2. Variable declarations
# 3. Function definitions
# 4. Main program logic
# 5. Function calls
```

---

## Language Features Summary

### ✅ **Implemented Features**
- **Basic Syntax**: Variables, expressions, statements
- **Functions**: Declaration, parameters, return types (optional)
- **Control Flow**: If-else, loops (for, while), switch/case, try/catch
- **Data Structures**: Arrays, objects with methods
- **Math Library**: 15+ mathematical functions and constants
- **Utility Library**: 20+ utility functions for debugging and data manipulation
- **Type System**: Clear type names ("Integer", "Float", "String", "Array", "Object")
- **Arrow Syntax**: Modern function return type annotations (`->`)
- **Switch Statements**: Expression-based case matching with default support
- **Error Handling**: Try-catch blocks with automatic error catching

### 🔄 **Implicit Functions (v1.2.4)**
- Optional parameter type annotations
- Optional return type annotations
- Implicit returns (returns 0 if no return statement)
- Dynamic return type inference

### 🧮 **Math Library (v1.3.0)**
- Basic operations: `abs`, `pow`, `sqrt`, `floor`, `ceil`
- Comparison: `min`, `max`
- Random: `random`, `randint`, `choice`
- Constants: `PI`, `E`, `INF`, `NAN`
- **Float Support**: Enhanced with floating-point arithmetic and mixed-type operations

### 🎯 **Utility Library (v1.3.2)**
- Debugging: `debug()`
- Type checking: `type()`, `is_num()`, `is_str()`, `is_arr()`, `is_obj()`
- String utilities: `str()`, `is_str()`, `find()`
- Data utilities: `copy()`, `has()`

### 🌊 **Float System (v1.4.0)**
- **Float Literals**: `3.14`, `0.5`, `.25`, `-2.5`
- **Float Arithmetic**: `+`, `-`, `*`, `/` with automatic type conversion
- **Mixed Operations**: Integer and float arithmetic with float result
- **Enhanced Math**: All math functions support float inputs and outputs
- **Float Display**: Proper formatting with 6 significant digits

### 📚 **Library Import System (v1.4.0)**
- **Import Syntax**: `use <library> as <alias>;`
- **Namespace Protection**: Built-in functions accessed via `alias.function()`
- **Available Libraries**: `math`, `utility`, `io`, `path_utils`, `env`, `args`, `process`, `text_utils`, `debug`, `types`, `polish`, `test`, `data`

### 🔧 **System Integration (v1.5.0)**
- **Path Utilities**: `join_path()`, `dirname()`, `basename()`, `normalize_path()`, `is_absolute()`, `relative_path()`
- **Environment Variables**: `get_env()`, `set_env()`, `list_env()`, `has_env()`
- **Command-Line Arguments**: `get_args()`, `get_arg()`, `arg_count()`, `parse_flags()`
- **Process Execution**: `execute()`, `get_pid()`, `get_cwd()`, `change_dir()`
- **Text Processing**: `read_lines()`, `write_lines()`, `read_csv()`, `write_csv()`
- **Enhanced Debugging**: `warn()`, `error()`, `assert()`, `start_timer()`, `end_timer()`, `get_stats()`

### 🎯 **Language Maturity (v1.6.0)**
- **Type System Foundation**: `typeof()`, `is_type()`, `cast()`, `enable_type_checking()`, `disable_type_checking()`, `enable_type_inference()`, `disable_type_inference()`, `set_strict_mode()`, `get_type_stats()`
- **Language Polish**: `enhance_lambda()`, `interpolate_string()`, `create_template()`, `enable_enhanced_lambdas()`, `disable_enhanced_lambdas()`, `enable_string_interpolation()`, `disable_string_interpolation()`, `get_polish_stats()`
- **Testing Framework**: `describe()`, `it()`, `expect()`, `assert()`, `assert_equals()`, `start_benchmark()`, `end_benchmark()`, `get_test_stats()`, `reset_tests()`
- **Advanced Data Structures**: `create_linked_list()`, `create_binary_tree()`, `create_hash_table()`, `create_priority_queue()`, `quicksort()`, `binary_search()`, `get_data_stats()`, `reset_data_structures()`

---

## Grammar Notes

1. **Whitespace**: Myco is whitespace-insensitive except within string literals
2. **Semicolons**: All statements must end with semicolons
3. **Type System**: Currently supports `int`, `float`, `string`, `array`, `object` as type specifiers
4. **Implicit Functions**: Function parameters and return types are optional
5. **Built-in Functions**: Available globally without import statements
6. **Memory Management**: Automatic memory management with tracking capabilities

---

## Future Extensions

### ✅ **Completed Features**
- **v1.4.0**: Float Support, Library Import System, True/False Keywords, File I/O
- **v1.5.0**: System Integration (Path Utilities, Environment Variables, Command-Line Args, Process Execution, Text Processing, Enhanced Debugging)
- **v1.6.0**: Language Maturity (Type System Foundation, Language Polish, Testing Framework, Advanced Data Structures)

### 🚀 **Planned for v1.7.0+**
- **Package Management**: Package manager with dependency resolution
- **Package Registry**: Community library ecosystem
- **Development Tools**: IDE support and enhanced tooling
- **Async Programming**: Asynchronous operations and concurrency
- **Networking**: HTTP, WebSocket, and network protocols
- **Database Integration**: Database connectivity and ORM features

---

*This BNF grammar specification is current as of Myco v1.6.0 and will be updated with each new language version.*
