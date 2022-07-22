// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

/*

1. Special constants
  min/max ± ∆
  Small numbers, required for peephole optimizations
  Based on context
  Bit blocks + end bits
  Reuse existing constants
2. Binary op replacement not only within a subset
3. Typecast operations
4. Create probabilities to control things that happen
  Swarm testing
5. Intrinsics / function calls
6. Insert arguments to function, reuse them
7. Swap/replace operands of different instructions
8. support more attributes
9. Randomly move instructions
10. remove void call or invoke
*/

#include "assert.h"
#include "llvm_util/llvm2alive.h"
#include "llvm_util/llvm_optimizer.h"
#include "smt/smt.h"
#include "tools/mutator-utils/mutator.h"
#include "tools/transform.h"
#include "util/version.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <utility>

using namespace tools;
using namespace util;
using namespace std;
using namespace llvm_util;

#define LLVM_ARGS_PREFIX ""
#define ARGS_SRC_TGT
#define ARGS_REFINEMENT
#include "llvm_util/cmd_args_list.h"

namespace {
llvm::cl::OptionCategory mutatorArgs("Mutator options");

llvm::cl::opt<string> testfile(llvm::cl::Positional,
                               llvm::cl::desc("<inputTestFile>"),
                               llvm::cl::Required,
                               llvm::cl::value_desc("filename"),
                               llvm::cl::cat(mutatorArgs));

}; // namespace

std::shared_ptr<llvm::Module> openInputFile(const string &inputFile,
                                            llvm::LLVMContext &context) {
  llvm::ExitOnError ExitOnErr;
  auto MB =
      ExitOnErr(errorOrToExpected(llvm::MemoryBuffer::getFile(inputFile)));
  llvm::SMDiagnostic Diag;
  auto pm = getLazyIRModule(std::move(MB), Diag, context,
                            /*ShouldLazyLoadMetadata=*/true);
  if (!pm) {
    Diag.print("", llvm::errs(), false);
    return nullptr;
  }
  ExitOnErr(pm->materializeAll());
  return std::move(pm);
}

void handle(std::shared_ptr<llvm::Module> ptr);

int main(int argc, char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);
  llvm::EnableDebugBuffering = true;
  llvm::llvm_shutdown_obj llvm_shutdown; // Call llvm_shutdown() on exit.

  std::string Usage =
      R"EOF(Alive2 stand-alone LLVM test mutator:
version )EOF";
  Usage += alive_version;

  // llvm::cl::HideUnrelatedOptions(alive_cmdargs);
  llvm::cl::HideUnrelatedOptions(mutatorArgs);
  llvm::cl::ParseCommandLineOptions(argc, argv, Usage);
  llvm::LLVMContext context;
  std::ofstream nout("/dev/null");
  out = &nout;
  report_dir_created = false;

  std::shared_ptr<llvm::Module> pm = openInputFile(testfile, context);
  handle(pm);
  return 0;
}

llvm::StringSet<> getRelatedFunctionNames(llvm::Function *func) {
  llvm::StringSet<> result, queue;

  llvm::Module *module = func->getParent();
  llvm::StringMap<llvm::StringSet<>> edges;

  for (auto fit = module->begin(); fit != module->end(); ++fit) {
    edges.insert(std::make_pair(fit->getName(), llvm::StringSet<>()));
  }

  for (auto fit = module->begin(); fit != module->end(); ++fit) {
    for (auto use_it = fit->use_begin(); use_it != fit->use_end(); ++use_it) {
      llvm::User *user = use_it->getUser();
      if (llvm::isa<llvm::Instruction>(user)) {
        llvm::Function *userFunc = ((llvm::Instruction *)user)->getFunction();
        assert(edges.find(userFunc->getName()) != edges.end() &&
               "u node should be in the graph");
        edges[userFunc->getName()].insert(fit->getName());
      }
    }
  }

  result.insert(func->getName());
  queue.insert(func->getName());

  while (!queue.empty()) {
    llvm::StringRef str = queue.begin()->getKey();
    llvm::Function *func = module->getFunction(str);

    assert(func != nullptr && "func in BFS cannot be nullptr");
    queue.erase(queue.begin());

    llvm::StringSet<> &adjEdges = edges[func->getName()];
    for (const auto &edge : adjEdges) {
      if (!result.contains(edge.getKey())) {
        queue.insert(edge.getKey());
        result.insert(edge.getKey());
      }
    }
  }
  return result;
}

static void updateConstantExprsRecursively(
    llvm::Use *use, llvm::ValueToValueMapTy &VMap,
    llvm::SmallPtrSet<llvm::Constant *, 32> &ConstantExprVisited, llvm::Module* newModule) {
  llvm::Constant *v = llvm::dyn_cast<llvm::Constant>(use->get());
  if (v == nullptr) {
    return;
  }

  if (!ConstantExprVisited.insert(v).second)
    return;

  SmallVector<std::pair<llvm::Constant *,llvm::Use&>, 16> Stack;
  Stack.push_back({v,*use});

  while (!Stack.empty()) {
    auto p=Stack.pop_back_val();
    llvm::Constant *C = p.first;
    llvm::Use& U=p.second;
    assert(U.get()==C&&"use should get right val");

    // Check this constant expression, skip this for now
    //if (const auto *CE = dyn_cast<ConstantExpr>(C))
    //  CE;

    if (const auto *GV = dyn_cast<GlobalValue>(C)) {
      llvm::errs()<<"found GV\n";
      GV->print(llvm::errs());
      llvm::errs()<<"\nGV end\n";
      // Global Values get visited separately, but we do need to make sure
      // that the global value is in the correct module
      if(auto it=VMap.find(GV);it!=VMap.end()){
        llvm::errs()<<"Replaced with\n";
        it->second->print(llvm::errs());
        const auto *newGV = dyn_cast<GlobalValue>(it->second);
        llvm::errs()<<"\n";
        llvm::errs()<<(GV->getParent()==newModule)<<"\n";
        llvm::errs()<<(newGV->getParent()==newModule)<<"\n";
        llvm::errs()<<(newGV->getParent()==GV->getParent())<<"\n";
        U.set(it->second);
      }
      continue;
    }

    // Visit all sub-expressions.
    for (Use &U : C->operands()) {
      auto *OpC = dyn_cast<Constant>(U);
      if (!OpC)
        continue;
      if (!ConstantExprVisited.insert(OpC).second)
        continue;
      Stack.push_back({OpC,U});
    }
  }
}

std::unique_ptr<llvm::Module>
cloneModuleWithOnlyFunction(std::shared_ptr<llvm::Module> module,
                            llvm::Function *func) {
  llvm::StringSet<> result = getRelatedFunctionNames(func);
  llvm::SmallVector<llvm::Function *> tmpFuncs;
  for (auto fit = module->begin(); fit != module->end(); ++fit) {
    if (!result.contains(fit->getName())) {
      tmpFuncs.push_back(&*fit);
    }
  }

  for (size_t i = 0; i < tmpFuncs.size(); ++i) {
    tmpFuncs[i]->removeFromParent();
  }

  llvm::ValueToValueMapTy VMap;
  llvm::SmallPtrSet<llvm::Constant*, 32> ConstantExprVisited;
  std::unique_ptr<llvm::Module> newModule = llvm::CloneModule(*module, VMap);
  llvm::Function* oldFunc=module->getFunction("_Z5funcBi");
  llvm::Function* newFunc=newModule->getFunction("_Z5funcBi");
  llvm::errs()<<oldFunc<<"old func\n";
  llvm::errs()<<newFunc<<"new func\n";
  llvm::errs()<<"AAA\n";
  //llvm::errs()<<(VMap.find(oldFunc)->second==newFunc)<<"val test\n";

  for (size_t i = 0; i < tmpFuncs.size(); ++i) {
    module->getFunctionList().push_back(tmpFuncs[i]);
  }

  llvm::errs() << "Global\n";
  for (auto glb_it = newModule->global_begin();
       glb_it != newModule->global_end(); ++glb_it) {
    glb_it->print(llvm::errs());
    llvm::errs() << "\n";
    glb_it->getInitializer()->print(llvm::errs());
    llvm::errs() << "\nOper\n";
    for (auto op_it = glb_it->op_begin(); op_it != glb_it->op_end(); ++op_it) {
      llvm::Value *useVal = op_it->get();
      useVal->print(llvm::errs());
      updateConstantExprsRecursively(&*op_it,VMap,ConstantExprVisited,newModule.get());
      llvm::errs() << "\n" << (llvm::isa<llvm::ConstantExpr>(&*useVal)) << "\n";
    }
    llvm::errs() << "\nOper end\n";
  }
  llvm::errs() << "Global end\n";

  return newModule;
}

void handle(std::shared_ptr<llvm::Module> ptr) {

  for (auto fit = ptr->begin(); fit != ptr->end(); ++fit) {
    std::unique_ptr<llvm::Module> newModule =
        cloneModuleWithOnlyFunction(ptr, &*fit);
    llvm::errs() << "veirfy result: " << verifyModule(*newModule, nullptr)
                 << "AAAAAAAAAAAA\n";
    verifyModule(*newModule, &llvm::errs());
    for (auto glb_it = ptr->global_begin(); glb_it != ptr->global_end();
         glb_it++) {
      glb_it->print(llvm::errs());

      llvm::errs() << "\n";
    }
    break;
  }
}
