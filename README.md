#LLVM JIT Compiler

A lightweight **Just-In-Time (JIT) compiler** built using [LLVM](https://llvm.org/).  
This project demonstrates how to generate, optimize, and execute code at runtime using the LLVM libraries.

---

## ✨ Features
- Uses **LLVM ORC JIT** for runtime code execution  
- Supports **dynamic compilation** of arithmetic expressions  
- Example integration with **LLVM IR generation**  
- Modular structure for future extensions (optimizations, custom passes, etc.)  

---

## 📂 Project Structure
```bash
llvm-JIT-compiler/
│── src/                # Source code
│   ├── jit.cpp         # JIT driver code
│   ├── codegen.cpp     # LLVM IR generation
│   └── ...
│── include/            # Header files
│── test/               # Example tests
│── CMakeLists.txt      # Build configuration
└── README.md           # Project documentation
