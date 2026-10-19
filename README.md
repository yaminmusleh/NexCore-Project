# 🚀 NexCore Compiler

![Language](https://shields.io)
![Grammar](https://shields.io)
![Build](https://shields.io)

**NexCore** is a custom programming language compiler utilizing an `.atg` (Attributed Grammar) definition configured for compiler generators like Coco/R. It parses high-level procedural syntax into an Abstract Syntax Tree (AST) represented via structured C++ nodes.

---

## 🗺️ Table of Contents
- [✨ Key Features](#-key-features)
- [🔤 Lexical Specifications](#-lexical-specifications)
  - [Characters](#characters)
  - [Tokens & Data Types](#tokens--data-types)
- [📝 Keywords & Syntax Guide](#-keywords-syntax-guide)
- [🧱 Abstract Syntax Tree (AST) Structure](#-abstract-syntax-tree-ast-structure)
- [💾 Example Nex Program](#-example-nex-program)

---

## ✨ Key Features
* **Distinct Control Flow Keywords:** Utilizes unique conditional keywords (`iff`, `otherwise`, `whilst`).
* **Strong Operator Precedence:** Deeply nested expression rules spanning Logical OR down to Unary operators.
* **Native C++ AST Nodes:** Built-in semantic actions directly constructing an AST (e.g., `make_let`, `make_if`, `make_while`).
* **Nested Comment Support:** Full lexical handling of single-line (`//`) and nested multi-line (`/* ... */`) comments.

---

## 🔤 Lexical Specifications

### Characters
* **`letter`**: `A-Z`, `a-z`
* **`digit`**: `0-9`
* **`stringChar`**: Any printable character excluding `"`, `\r`, and `\n`.

### Tokens & Data Types

| Token | Representation / Pattern | Example |
| :--- | :--- | :--- |
| **`identifier`** | `letter { letter \| digit \| "_" }` | `my_variable1` |
| **`int_lit`** | `digit { digit }` | `42` |
| **`float_lit`** | `digit { digit } "." digit { digit } "f"` | `3.14f` |
| **`double_lit`** | `digit { digit } "." digit { digit } "d"` | `0.001d` |
| **`string_lit`** | `'"' { stringChar } '"'` | `"Hello, Nex!"` |

---

## 📝 Keywords & Syntax Guide

### Variable Declaration & Assignment
Variables are explicitly declared using the `set` keyword, and reassigned directly via their identifiers.
```rust
set score = 100;    // Declaration
score = 150;        // Reassignment
```

### Conditionals (`iff` / `otherwise`)
Conditional logic uses `iff` for evaluation, with optional `otherwise` branches that support either a secondary block or direct chaining into another `iff`.
```rust
iff (score >= 100) {
    print("Excellent!");
} otherwise iff (score >= 50) {
    print("Good job!");
} otherwise {
    print("Keep trying!");
}
```

### Loops (`for` / `whilst`)
Standard indefinite looping relies on `whilst`. Definite multi-expression looping relies on `for`.
```rust
// Whilst Loop
whilst (count < 10) {
    count = count + 1;
}

// For Loop
for (set i = 0; i < 5; i = i + 1) {
    print(i);
}
```

---

## 🧱 Abstract Syntax Tree (AST) Structure
The production grammar converts parsed code tokens into robust `NodeStmnt` and `NodeExpr` structural objects inside a `program.statements` vectors. 

<details>
<summary>🔍 Click to view full Expression Hierarchy (Highest to Lowest Precedence)</summary>

1. **`OrExpr`** (`||`)
2. **`AndExpr`** (`&&`)
3. **`EqualityExpr`** (`==`, `!=`)
4. **`RelationalExpr`** (`<`, `<=`, `>`, `>=`)
5. **`AddExpr`** (`+`, `-`)
6. **`MultExpr`** (`*`, `/`, `%`)
7. **`UnaryExpr`** (`!`, `-`, `+`)
</details>

---

## 💾 Example Nex Program

Here is an example program demonstrating standard syntax processing supported by the `Nex` compiler production engine:

```rust
// Initialize data types
set greeting = "Compiler Execution Started";
print(greeting);

set threshold = 10.5f;
set index = 0;

whilst (index < 5) {
    iff (index == 3) {
        print("Index reached three");
    }
    index = index + 1;
}

exit(0);
```

## NexCore Studio (desktop editor)

A Qt 6 desktop editor with Build, Run and Stop for the NexCore compiler is in
[`studio/`](studio/README.md). It is a separate CMake project; building and using
the compiler itself is unchanged.
