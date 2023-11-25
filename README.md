Instructions of building Alive-mutate 
------

Prerequisites
-------------
To build Alive2 you need recent versions of:
* [cmake](https://cmake.org)
* [gcc](https://gcc.gnu.org)/[clang](https://clang.llvm.org)
* [re2c](https://re2c.org/)
  + `sudo apt-get install re2c`
* [Z3](https://github.com/Z3Prover/z3)
```
git clone https://github.com/Z3Prover/z3
cd z3
python scripts/mk_make.py
cd build
make
sudo make install
```
* [LLVM](https://github.com/llvm/llvm-project)
* [hiredis](https://github.com/redis/hiredis) (optional, needed for caching)

Building LLVM
--------
```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
cmake -GNinja -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_EH=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="llvm;clang" ../llvm
ninja
```

Building alive-mutate
--------

```
git clone -b artifacts https://github.com/Hatsunespica/alive2.git
cd alive2
mkdir build
cd build
cmake -GNinja -DCMAKE_PREFIX_PATH=~/GitRepo/llvm-project/build -DBUILD_TV=1 -DCMAKE_BUILD_TYPE=Release ..
ninja
```

If CMake cannot find the Z3 include directory (or finds the wrong one) pass
the ``-DZ3_INCLUDE_DIR=/path/to/z3/include`` and ``-DZ3_LIBRARIES=/path/to/z3/lib/libz3.so`` arguments to CMake.

Running experiments
------
See https://github.com/Hatsunespica/alive2/tree/artifacts/benchmark
