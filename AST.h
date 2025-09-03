#pragma once


#include<string>
#include<memory>
#include"lexer.h"
#include<map>
#include<cassert>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"


using llvm::Value;
class Ast{
public:
    virtual ~Ast()=default;
    virtual Value *codegen()=0;

};
 
class NumAst : public Ast{
    double value;
public:
    NumAst(double val) : value(val){};
    Value *codegen() override;
};

class VariableAst : public Ast{
    const std::string variable;

public:
    VariableAst(const std::string var) : variable(var){};
    Value *codegen() override;


 
};

class BinaryAst : public Ast{
    char op;
    std::unique_ptr<Ast> right,left;
    
public:   
    BinaryAst(char operand,std::unique_ptr<Ast> right,std::unique_ptr<Ast> left) : op(operand),
    right(std::move(right)),left(std::move(left)){};
    Value *codegen() override;
};


//this class is for storing the function call
class CallexprAst : public Ast{
    std::string caller;
    std::vector<std::unique_ptr<Ast>> Args;
public:
    CallexprAst(const std::string &caller,std::vector<std::unique_ptr<Ast>> Args) : caller(caller),Args(std::move(Args)){};
    Value *codegen() override;
};


//this class is for storing the function definition
class PrototypeAst : public Ast{
    const std::string name;
    std::vector<std::string> Args;
public:
    PrototypeAst(const std::string &name,std::vector<std::string> Args) : name(name),Args(Args){};
    llvm::Function *codegen() override;
    const std::string &getName() const { return name; }

};


//this function is for body of the function
class FuncbodyAst : public Ast{
    std::unique_ptr<PrototypeAst> proto;
    std::unique_ptr<Ast> body;
public:
    FuncbodyAst(std::unique_ptr<PrototypeAst> proto,std::unique_ptr<Ast> body) : proto(std::move(proto)),body(std::move(body)){};
    llvm::Function *codegen() override;
};

class IfExprAst : public Ast{
    std::unique_ptr<Ast> Cond,then,Else;

public:
    IfExprAst(std::unique_ptr<Ast> Cond,std::unique_ptr<Ast> Then,std::unique_ptr<Ast> Else) : Cond(std::move(Cond)),then(std::move(then)),Else(std::move(Else)){}
    llvm::Value *codegen() override;

}


