cmake_minimum_required(VERSION 3.15)
project(ToyCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LLVM Setup
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCES
    lexer.cpp
    AST.cpp
)

add_executable(toy ${SOURCES})

# Use only required LLVM components (remove AArch64)
target_link_libraries(toy PRIVATE
  LLVMCore
  LLVMSupport
  LLVMOrcJIT
  LLVMExecutionEngine
  LLVMBitWriter
  LLVMIRReader
  LLVMPasses
  LLVMScalarOpts
  LLVMAnalysis
  LLVMTransformUtils
)

target_include_directories(toy PRIVATE ${LLVM_INCLUDE_DIRS})
