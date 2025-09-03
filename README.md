# LLVM Kaleidoscope JIT Compiler

An implementation of the **Kaleidoscope language JIT compiler** from the [LLVM Tutorial](https://llvm.org/docs/tutorial/).  
This project demonstrates how to build a toy programming language from scratch using LLVM’s APIs — including a **parser, AST, IR generation, optimizations, and a JIT execution engine**.

---

## Features
- **Toy language (Kaleidoscope)** implementation  
- **JIT compilation** using LLVM **ORC JIT**  
- **Lexer & Parser** for handling custom syntax  
- **Abstract Syntax Tree (AST)** for semantic representation  
- **LLVM IR generation** using IRBuilder  
- **Optimizations**: constant folding, function inlining, dead code elimination (via LLVM passes)  
- Interactive **REPL (Read–Eval–Print Loop)**  

---

## Project Structure
```bash
llvm-JIT-compiler/
│── src/
│   ├── lexer.cpp        # Lexical analysis
│   ├── parser.cpp       # Parser & AST
│   ├── codegen.cpp      # LLVM IR generation
│   ├── jit.cpp          # ORC JIT engine
│   └── main.cpp         # Driver / REPL
│── include/             # Header files
│── test/                # Example programs
│── CMakeLists.txt       # Build configuration
└── README.md            # Project documentation
