#include"AST.h"

#include "llvm/IR/LLVMContext.h"

static std::unique_ptr<llvm::LLVMContext> Context;

static std::unique_ptr<llvm::IRBuilder<>> builder;

static std::unique_ptr<llvm::Module> Module;

static std::map<std::string,Value *> Namedvalues;


int ret(int n){

    return __builtin_popcount(n);

}


std::unique_ptr<Ast> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

Value *LogErrorV(const char *Str) {
    LogError(Str);
    return nullptr;
}

Value *NumAst::codegen(){
    return llvm::ConstantFP::get(*Context,APFloat(P));
}

Value *BinaryAst::codegen(){
    Value* lhs=left->codegen();
    Value* rhs=right->codegen();
    switch(op){
    case '+':
        return builder->CreateAdd(lhs,rhs,"addtmp");
    case '-':
        return builder->CreateSub(lhs,rhs,"subtmp");
    case '*':
        return builder->CreateMul(lhs,rhs,"multmp");
    case '/':
        return builder->CreateFDiv(lhs,rhs,"divtmp");
    case '<':
        lhs=builder->CreateFCmpULT(lhs,rhs,"cmptmp");
        return builder->CreateUIToFP(lhs,llvm::Type::getDoubleTy(*Context),"boolcmp");
    default:
        return LogErrorV("Invalid values");
    }

}

Value *CallexprAst::codegen(){
    llvm::Function* callee=Module->getFunction(caller);
    if (!callee)
    {
        return LogErrorV("Unknown arguments");
    }
    if(callee->arg_size()!=Args.size()){
        return LogErrorV("Incorrect arguments passed");

    }
    std::vector<Value *> ArgsV;
    for(unsigned i=0;i<Args.size();i++){
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
        {
            return nullptr;
        }
        


    }
    return builder->CreateCall(callee,ArgsV,"Calltmp");

    

}

llvm::Function *PrototypeAst::codegen(){
    std::vector<llvm::Type*> doubles(Args.size(),llvm::Type::getDoubleTy(*Context));
    llvm::FunctionType* FT=llvm::FunctionType::get(llvm::Type::getDoubleTy(*Context),doubles,false);
    llvm::Function *F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,Name,Module.get());
    unsigned i=0;
    for(auto &Arg:F->args()){
        Arg.setName(Args[i++]);

    }
    return F;
}

llvm::Function *FuncbodyAst::codegen(){
    llvm::Function *test=Module->getFunction(proto->getName());
    if (!test)
       test = proto->codegen();

    if (!test)
        return nullptr;

    if (!test->empty())
        return (llvm::Function*)LogErrorV("Function cannot be redefined.");
    
    llvm::BasicBlock *BB=llvm::BasicBlock::Create(*Context,"body",test);
    builder->SetInsertPoint(BB);
    
    Namedvalues.clear();
    for(auto &args:test->args()){
        Namedvalues[std::string(args.getName())]=&args;
    }

    if(llvm::Value *RetVal=body->codegen()){
        builder->CreateRet(RetVal);

        verifyFunction(*test);

        return test;

    }

    test->eraseFromParent();
    return nullptr;

}

static void InitializeModule() {
    // Open a new context and module.
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>("my cool jit", *Context);
  
    // Create a new builder for the module.
    builder = std::make_unique<llvm::IRBuilder<>>(*Context);
}



