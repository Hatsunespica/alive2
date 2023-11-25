Alive2
======

Alive2 consists of several libraries and tools for analysis and verification
of LLVM code and transformations.
Alive2 includes the following libraries:
* Alive2 IR
* Symbolic executor
* LLVM → Alive2 IR converter
* Refinement check (aka optimization verifier)
* SMT abstraction layer

Included tools:
* Alive drop-in replacement
* Translation validation plugins for clang and LLVM's `opt`
* Standalone translation validation tool: `alive-tv` ([online](https://alive2.llvm.org))
* Clang drop-in replacement with translation validation (`alivecc` and
  `alive++`)
* An LLVM IR interpreter that is UB precise (`alive-exec`)

For a technical introduction to Alive2, please see [our paper from
PLDI 2021](https://web.ist.utl.pt/nuno.lopes/pubs/alive2-pldi21.pdf).


WARNING
-------
Alive2 does not support inter-procedural transformations. Alive2 may produce
spurious counterexamples if run with such passes.


Prerequisites
-------------
To build Alive2 you need recent versions of:
* [cmake](https://cmake.org)
* [gcc](https://gcc.gnu.org)/[clang](https://clang.llvm.org)
* [re2c](https://re2c.org/)
* [Z3](https://github.com/Z3Prover/z3)
* [LLVM](https://github.com/llvm/llvm-project) (optional)
* [hiredis](https://github.com/redis/hiredis) (optional, needed for caching)


Building
--------

```
export ALIVE2_HOME=$PWD
export LLVM2_HOME=$PWD/llvm-project
export LLVM2_BUILD=$LLVM2_HOME/build
git clone git@github.com:AliveToolkit/alive2.git
cd alive2
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

If CMake cannot find the Z3 include directory (or finds the wrong one) pass
the ``-DZ3_INCLUDE_DIR=/path/to/z3/include`` and ``-DZ3_LIBRARIES=/path/to/z3/lib/libz3.so`` arguments to CMake.


Building and Running Translation Validation
--------

Alive2's `opt` and `clang` translation validation requires a build of LLVM with
RTTI and exceptions turned on.
LLVM can be built targeting X86 in the following way.  (You may prefer to add `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` to the CMake step if your default compiler is `gcc`.)
```
cd $LLVM2_HOME
mkdir build
cd build
cmake -GNinja -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_EH=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="llvm;clang" ../llvm
ninja
```

Alive2 should then be configured and built as follows:
```
cd $ALIVE2_HOME/alive2/build
cmake -GNinja -DCMAKE_PREFIX_PATH=$LLVM2_BUILD -DBUILD_TV=1 -DCMAKE_BUILD_TYPE=Release ..
ninja
```

Translation validation of one or more LLVM passes transforming an IR file on Linux:
```
$LLVM2_BUILD/bin/opt -load $ALIVE2_HOME/alive2/build/tv/tv.so -load-pass-plugin $ALIVE2_HOME/alive2/build/tv/tv.so -tv -instcombine -tv -o /dev/null foo.ll
```
For the new pass manager:
```
$LLVM2_BUILD/bin/opt -load $ALIVE2_HOME/alive2/build/tv/tv.so -load-pass-plugin $ALIVE2_HOME/alive2/build/tv/tv.so -passes=tv -passes=instcombine -passes=tv -o /dev/null $LLVM2_HOME/llvm/test/Analysis/AssumptionCache/basic.ll
```


On a Mac with the old pass manager:
```
$LLVM2_BUILD/bin/opt -load $ALIVE2_HOME/alive2/build/tv/tv.dylib -load-pass-plugin $ALIVE2_HOME/alive2/build/tv/tv.dylib -tv -instcombine -tv -o /dev/null foo.ll
```
On a Mac with the new pass manager:
```
$LLVM2_BUILD/bin/opt -load $ALIVE2_HOME/alive2/build/tv/tv.dylib -load-pass-plugin $ALIVE2_HOME/alive2/build/tv/tv.dylib -passes=tv -passes=instcombine -passes=tv -o /dev/null $LLVM2_HOME/llvm/test/Analysis/AssumptionCache/basic.ll
```
You can run any pass or combination of passes, but on the command line
they must be placed in between the two invocations of the Alive2 `-tv`
pass.


Translation validation of a single LLVM unit test, using lit:
```
$LLVM2_BUILD/bin/llvm-lit -vv -Dopt=$ALIVE2_HOME/alive2/build/opt-alive.sh $LLVM2_HOME/llvm/test/Transforms/InstCombine/canonicalize-constant-low-bit-mask-and-icmp-sge-to-icmp-sle.ll
```

The output should be:
```
-- Testing: 1 tests, 1 threads --
PASS: LLVM :: Transforms/InstCombine/canonicalize-constant-low-bit-mask-and-icmp-sge-to-icmp-sle.ll (1 of 1)
Testing Time: 0.11s
  Expected Passes    : 1
```

To run translation validation on all the LLVM unit tests for IR-level
transformations:

```
$LLVM2_BUILD/bin/llvm-lit -s -Dopt=$ALIVE2_HOME/alive2/build/opt-alive.sh $LLVM2_HOME/llvm/test/Transforms
```

We run this command on the main LLVM branch each day, and keep track of the results
[here](https://web.ist.utl.pt/nuno.lopes/alive2/).  To detect unsound transformations in a local run:

```
fgrep -r "(unsound)" $ALIVE2_HOME/alive2/build/logs/
```
LLVM Bugs Found by Alive2
--------

[BugList.md](BugList.md) shows the list of LLVM bugs found by Alive2.
