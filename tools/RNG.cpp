// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

#include "cache/cache.h"
#include "llvm_util/compare.h"
#include "llvm_util/llvm2alive.h"
#include "llvm_util/llvm_optimizer.h"
#include "llvm_util/utils.h"
#include "smt/smt.h"
#include "tools/transform.h"
#include "util/version.h"
#include "tools/mutator-utils/mutator.h"

#include "llvm/ADT/StringExtras.h"
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
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

using namespace tools;
using namespace util;
using namespace std;
using namespace llvm_util;

#define LLVM_ARGS_PREFIX ""

namespace {
llvm::cl::OptionCategory mutatorArgs("Mutator options");

llvm::cl::opt<long long> randomSeed(
    LLVM_ARGS_PREFIX "s",
    llvm::cl::value_desc("specify the seed of the random number generator"),
    llvm::cl::cat(mutatorArgs),
    llvm::cl::desc("specify the seed of the random number generator"),
    llvm::cl::init(-1));

llvm::cl::opt<int>
    numCopy(LLVM_ARGS_PREFIX "n",
            llvm::cl::value_desc("number of copies of test files"),
            llvm::cl::desc("specify number of copies of test files"),
            llvm::cl::cat(mutatorArgs), llvm::cl::init(-1));
};


std::vector<unsigned> RNGseeds;

int main(int argc, char **argv) {
  llvm::cl::HideUnrelatedOptions(mutatorArgs);
  llvm::cl::ParseCommandLineOptions(argc, argv);
  if (randomSeed >= 0) {
    Random::setSeed((unsigned)randomSeed);
    RNGseeds.resize(numCopy);
    for(int i=0;i<numCopy;++i){
      RNGseeds[i]=Random::getRandomUnsigned();
      cout<<RNGseeds[i]<<" ";
    }
  }
  return 0;
}

