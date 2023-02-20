#pragma once

// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

#include "smt/smt.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Function.h"
#include <ostream>
#include <unordered_set>
#include <sstream>
#include <fstream>

namespace llvm_util {

struct Verifier {
  llvm::TargetLibraryInfoWrapperPass &TLI;
  smt::smt_initializer &smt_init;
  std::ostream &out;
  unsigned num_correct = 0;
  unsigned num_unsound = 0;
  unsigned num_failed = 0;
  unsigned num_errors = 0;
  bool quiet = false;
  bool always_verify = false;
  bool print_dot = false;
  bool bidirectional = false;

  Verifier(llvm::TargetLibraryInfoWrapperPass &TLI,
           smt::smt_initializer &smt_init, std::ostream &out)
    : TLI(TLI), smt_init(smt_init), out(out) {}

  bool compareFunctions(llvm::Function &F1, llvm::Function &F2);
};

struct VerifierWithLogs : public Verifier{
  bool verbose = false;  

  std::ofstream &out_file;

  std::stringstream& logs;
  std::unordered_set<std::string>& logsFilter;
  unsigned randomSeed;

  VerifierWithLogs(llvm::TargetLibraryInfoWrapperPass &TLI,
           smt::smt_initializer &smt_init, std::ostream &out,std::ofstream &out_file,
           std::stringstream& logs,std::unordered_set<std::string>& logsFilter, unsigned seed):
           Verifier(TLI,smt_init,out),out_file(out_file),logs(logs),logsFilter(logsFilter),randomSeed(seed){};

  bool compareFunctions(llvm::Function &F1, llvm::Function &F2);
};

}
