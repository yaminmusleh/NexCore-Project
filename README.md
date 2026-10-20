# NexCore — Custom Programming Language & Compiler

<p align="center">
  **A custom programming language built with C++ and Coco/R**
  
  Explore the grammar, understand the compiler, and write programs using NexCore's distinctive syntax.
</p>

---

## Overview

**NexCore** is a custom programming language compiler project built around an attributed grammar (`.atg`) designed for compiler generators such as Coco/R. It parses NexCore source code and constructs an Abstract Syntax Tree (AST) using structured C++ nodes.

The project explores the core ideas behind programming language design, including lexical analysis, grammar rules, expression precedence, control flow, and syntax tree construction.

## Features

- **Custom language syntax** — distinctive keywords such as `set`, `iff`, `otherwise`, and `whilst`.
- **Expression parsing** — operator precedence covering logical, equality, relational, arithmetic, and unary expressions.
- **C++ AST construction** — semantic actions build statement and expression nodes.
- **Variables and assignment** — declare variables and update their values.
- **Conditional statements** — write branching logic using `iff` and `otherwise`.
- **Looping constructs** — use `whilst` and `for` loops.
- **Comments** — support for single-line and nested multi-line comments.
- **Desktop studio** — a Qt 6 editor project is available in `studio/`.

## Example

```rust
// Start a NexCore program
set greeting = "Hello, NexCore!";
print(greeting);

set index = 0;

whilst (index < 5) {
    iff (index == 3) {
        print("Reached three!");
    } otherwise {
        print(index);
    }

    index = index + 1;
}

exit(0);
```

## Language Syntax

### Variables

Use `set` to declare a variable and its initial value.

```rust
set score = 100;
score = 150;
```

### Conditions

Use `iff` and `otherwise` to control branching.

```rust
iff (score >= 100) {
    print("Excellent!");
} otherwise iff (score >= 50) {
    print("Good job!");
} otherwise {
    print("Keep trying!");
}
```

### Loops

Use `whilst` for condition-based repetition and `for` for a loop with initialization, condition, and update expressions.

```rust
whilst (count < 10) {
    count = count + 1;
}
```

```rust
for (set i = 0; i < 5; i = i + 1) {
    print(i);
}
```

## Lexical Specifications

| Element | Description | Example |
|---|---|---|
| Identifiers | Letters followed by letters, digits, or underscores | `my_variable1` |
| Integers | Sequences of digits | `42` |
| Floats | Decimal numbers ending in `f` | `3.14f` |
| Doubles | Decimal numbers ending in `d` | `0.001d` |
| Strings | Text enclosed in double quotes | `"Hello!"` |

## Operator Precedence

From lowest to highest precedence:

1. **Logical OR** — `||`
2. **Logical AND** — `&&`
3. **Equality** — `==`, `!=`
4. **Relational** — `<`, `<=`, `>`, `>=`
5. **Addition and subtraction** — `+`, `-`
6. **Multiplication, division, and modulo** — `*`, `/`, `%`
7. **Unary operators** — `!`, `-`, `+`

## Abstract Syntax Tree

The compiler's grammar uses semantic actions to construct C++ nodes representing parsed statements and expressions. The resulting program structure organizes statements under `program.statements`, with expression and statement node types representing the parsed source.

This structure forms the basis for further compiler development and language features.

## NexCore Studio

NexCore also includes a Qt 6 desktop editor project.

- **Location:** [`studio/`](studio/README.md)
- **Build system:** CMake
- **Purpose:** A desktop interface for working with the NexCore compiler

Refer to the Studio README for its specific build instructions and requirements.

## Project Structure

```text
NexCore-Project/
├── Grammar and compiler source files
├── studio/
│   └── README.md
└── README.md
```

*The exact source-file layout may evolve as the project develops.*

## Architecture

View the project's architecture diagram:

[**Explore the NexCore architecture diagram**](https://gitdiagram.com/yaminmusleh/nexcore-project)

## Project Goals

NexCore is a continuing exploration of programming language design and compiler development. Its goal is to provide a foundation for experimenting with custom syntax, parsing, AST construction, and desktop development tools.

---

## Copyright

**Copyright © 2026 Yamin Musleh. All rights reserved.**

NexCore and its original source code are the intellectual property of the author.

You may view, study, and make copies of this repository for personal or educational purposes, provided that the original copyright notice and attribution remain intact. You may not claim the project or its original code as your own, remove its attribution, or redistribute or republish the code as your own without prior written permission from the copyright holder.

Viewing or copying the code does not transfer ownership of the intellectual property. No broader permission to reuse or redistribute the project is granted by this notice.

