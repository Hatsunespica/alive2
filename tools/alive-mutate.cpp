// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

#include "llvm_util/llvm2alive.h"
#include "smt/smt.h"
#include "tools/transform.h"
#include "util/version.h"
#include "tools/mutator-utils/SingleLineMutator.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/CommandLine.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <chrono>


using namespace tools;
using namespace util;
using namespace std;


#define LLVM_ARGS_PREFIX ""
#define ARGS_SRC_TGT
#define ARGS_REFINEMENT


namespace {

  llvm::cl::opt<string> testfile(llvm::cl::Positional,
    llvm::cl::desc("input test file"),
    llvm::cl::Required, llvm::cl::value_desc("filename"));

  llvm::cl::opt<string> outputFolder(llvm::cl::Positional,
    llvm::cl::desc("output file folder"),
    llvm::cl::Required, llvm::cl::value_desc("folder"));

  llvm::cl::opt<int> numCopy("n",llvm::cl::desc("number copies of test files"),llvm::cl::init(-1));

  llvm::cl::opt<int> timeElapsed("t",llvm::cl::desc("seconds of mutator should run"),llvm::cl::init(-1));
  llvm::cl::opt<bool> verbose("-v",llvm::cl::desc("verbose mode"));

  llvm::ExitOnError ExitOnErr;

  filesystem::path inputPath,outputPath;

  // adapted from llvm-dis.cpp
  std::unique_ptr<llvm::Module> openInputFile(llvm::LLVMContext &Context,
                                              const string &InputFilename) {
    auto MB =
      ExitOnErr(errorOrToExpected(llvm::MemoryBuffer::getFile(InputFilename)));
    llvm::SMDiagnostic Diag;
    auto M = getLazyIRModule(move(MB), Diag, Context,
                            /*ShouldLazyLoadMetadata=*/true);
    if (!M) {
      Diag.print("", llvm::errs(), false);
      return 0;
    }
    ExitOnErr(M->materializeAll());
    return M;
  }
  /*llvm::Function *findFunction(llvm::Module &M, const string &FName) {
    for (auto &F : M) {
      if (F.isDeclaration())
        continue;
      if (FName.compare(F.getName()) != 0)
        continue;
      return &F;
    }
    return 0;
  }*/
}

void copyMode(),timeMode();
bool isValidInputPath(),isValidOutputPath();
string getOutputFile(int ith);

int main(int argc, char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);
  llvm::EnableDebugBuffering = true;
  llvm::llvm_shutdown_obj llvm_shutdown; // Call llvm_shutdown() on exit.
  llvm::LLVMContext Context;

  std::string Usage =
      R"EOF(Alive2 stand-alone llvm test mutator )EOF";
  Usage += alive_version;
  Usage += R"EOF(empty for now)EOF";

  //llvm::cl::HideUnrelatedOptions(alive_cmdargs);
  llvm::cl::ParseCommandLineOptions(argc, argv, Usage);
  if(numCopy<0&&timeElapsed<0){
    cerr<<"Please specify either number of copies or running time!\n";
    return -1;
  }else if(!isValidInputPath()){
    cerr<<"Input file does not exist!\n";
    return -1;
  }else if(!isValidOutputPath()){
    cerr<<"Output folder does not exist!\n";
    return -1;
  }

  cout<<testfile<<' '<<numCopy<<' '<<timeElapsed<<"\n";
  
  if(numCopy>0){
    copyMode();
  }else if(timeElapsed>0){
    timeMode();
  }

  auto M1 = openInputFile(Context, testfile);
  if (!M1.get()) {
    cerr << "Could not read bitcode from '" << testfile << "'\n";
    return -1;
  }

  for(const llvm::Function& func:*M1){
    func.print(llvm::outs());
  }
  return 0;
}

bool isValidInputPath(){
  bool result=filesystem::status(string(testfile)).type()==filesystem::file_type::regular;
  if(result){
    inputPath=filesystem::path(string(testfile));
  }
  return result;
}

bool isValidOutputPath(){
  bool result= filesystem::status(string(outputFolder)).type()==filesystem::file_type::directory;
  if(result){
    outputPath=filesystem::path(string(outputFolder));
  }
  return result;
}

string getOutputFile(int ith){
  static string templateName=string(outputFolder)+inputPath.stem().string();
  return templateName+to_string(ith)+".ll";
}

void copyMode(){
  SingleLineMutator mutator;
  if(mutator.openInputFile(testfile)&&mutator.init()){
    for(int i=1;i<=numCopy;++i){
      if(verbose){
        std::cout<<"Currently generating "+to_string(i)+"th copies\n";
      }
      mutator.generateTest(getOutputFile(i));
    }
  }else{
    cerr<<"Mutator error! Please check your testfile\n";
  }
}

void timeMode(){
  SingleLineMutator mutator;
  if(mutator.openInputFile(testfile)&&mutator.init()){
    std::chrono::duration<double> sum=std::chrono::duration<double>::zero();
    int cnt=1;
    while(sum.count()<timeElapsed){
      auto t_start = std::chrono::high_resolution_clock::now();
      mutator.generateTest(getOutputFile(cnt));
      auto t_end = std::chrono::high_resolution_clock::now();
      if(verbose){
        std::cout<<"Generted "+to_string(cnt)+"th copies in "+to_string((t_end-t_start).count())+" seconds\n";
      }
      sum+=t_end-t_start;
      ++cnt;
    }
  }else{
    cerr<<"Mutator error! Please check your testfile\n";
  }
}
