#include"AST.h"
#include"JIT.h"
#include"lexer.h"

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::map<std::string, Value *> NamedValues;
// static std::unique_ptr<KaleidoscopeJIT > TheJIT;
static std::unique_ptr<llvm::FunctionPassManager> TheFPM;
static std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
static std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
static std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
static std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
static std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
static std::unique_ptr<llvm::StandardInstrumentations> TheSI;
static std::map<std::string, std::unique_ptr<PrototypeAst>> FunctionProtos;
static std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;

static llvm::ExitOnError ExitOnErr;



  
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }


static std::map<char, int> BinopPrecedence;


static int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}




std::unique_ptr<Ast> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}
std::unique_ptr<PrototypeAst> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

static std::unique_ptr<Ast> ParseExpression();

static std::unique_ptr<Ast> ParseNumberExpr() {
  auto Result = std::make_unique<NumAst>(NumVal);
  getNextToken(); 
  return std::move(Result);
}


static std::unique_ptr<Ast> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}

static std::unique_ptr<Ast> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken(); // eat identifier.

  if (CurTok != '(') 
    return std::make_unique<VariableAst>(IdName);


  getNextToken(); 
  std::vector<std::unique_ptr<Ast>> Args;
  if (CurTok != ')') {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallexprAst>(IdName, std::move(Args));
}

static std::unique_ptr<Ast> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("unknown token when expecting an expression");
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}

static std::unique_ptr<Ast> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<Ast> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok;
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

   
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }


    LHS =
        std::make_unique<BinaryAst>(BinOp, std::move(LHS), std::move(RHS));
  }
}


static std::unique_ptr<Ast> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAst> ParsePrototype() {
  if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  getNextToken();

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  // success.
  getNextToken(); // eat ')'.

  return std::make_unique<PrototypeAst>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FuncbodyAst> ParseDefinition() {
  getNextToken(); // eat def.
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FuncbodyAst>(std::move(Proto), std::move(E));
  return nullptr;
}

/// toplevelexpr ::= expression
static std::unique_ptr<FuncbodyAst> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAst>("__anon_expr",
                                                std::vector<std::string>());
    return std::make_unique<FuncbodyAst>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAst> ParseExtern() {
  getNextToken(); // eat extern.
  return ParsePrototype();
}
static std::unique_ptr<llvm::LLVMContext> Context;

static std::unique_ptr<llvm::IRBuilder<>> builder;

static std::unique_ptr<llvm::Module> Module;

static std::map<std::string,Value *> Namedvalues;


// std::unique_ptr<Ast> LogError(const char *Str) {
//     fprintf(stderr, "Error: %s\n", Str);
//     return nullptr;
// }

Value *LogErrorV(const char *Str) {
    LogError(Str);
    return nullptr;
}

Value *NumAst::codegen(){
    return llvm::ConstantFP::get(*Context,llvm::APFloat(value));
}

Value *VariableAst::codegen(){
  Value *varname=Namedvalues[variable];
  if(!varname){
    return LogErrorV("invalid arguments");
  }
  return varname;
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
    llvm::Function *F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,name,Module.get());
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
        TheFPM->run(*test,*TheFAM);

        return test;

    }

    test->eraseFromParent();
    return nullptr;

}


// static ExitOnError ExitOnErr;



static void InitializeModule() {
  // Open a new context and module.
  Context = std::make_unique<llvm::LLVMContext>();
  Module = std::make_unique<llvm::Module>("my cool jit", *Context);

  // Create a new builder for the module.
  builder = std::make_unique<llvm::IRBuilder<>>(*Context);
  TheFPM = std::make_unique<llvm::FunctionPassManager>();
  TheLAM = std::make_unique<llvm::LoopAnalysisManager>();
  TheFAM = std::make_unique<llvm::FunctionAnalysisManager>();
  TheCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
  TheMAM = std::make_unique<llvm::ModuleAnalysisManager>();
  ThePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
  TheSI = std::make_unique<llvm::StandardInstrumentations>(*TheContext, /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes.
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->addPass(llvm::InstCombinePass());
  // Reassociate expressions.
  TheFPM->addPass(llvm::ReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->addPass(llvm::GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->addPass(llvm::SimplifyCFGPass());

  // Register analysis passes used in these transform passes.
  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(llvm::errs());
      fprintf(stderr, "\n");
      ExitOnErr(TheJIT->addModule(
        llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
      InitializeModule();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(llvm::errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    if (FnAST->codegen()) {
      // Create a ResourceTracker to track JIT'd memory allocated to our
      // anonymous expression -- that way we can free it after executing.
      auto RT = TheJIT->getMainJITDylib().createResourceTracker();

      auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
      InitializeModule();

      // Search the JIT for the __anon_expr symbol.
      auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
      assert(ExprSymbol.getAddress() && "Function not found");

      // Get the symbol's address and cast it to the right type (takes no
      // arguments, returns a double) so we can call it as a native function.
      double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();
      fprintf(stderr, "Evaluated to %f\n", FP());

      // Delete the anonymous expression module from the JIT.
      ExitOnErr(RT->remove());
    }
  }
}      


/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}








int main() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

  InitializeModule();

  // Run the main "interpreter loop" now.
  MainLoop();
  Module->print(llvm::errs(),nullptr);

  return 0;
}