Instructions on benchmakring alive-mutate
=====


# File organization
* Subfolder fuzzing (This folder is used for showing alive-mutate can fuzze variant LLVM IR files)
  + run.sh:  By running this script, alive-mutate will be executed for every files in tests with the arugment specified in this script.
  + tests: a subfolder used to hold all IR files as fuzzing input.
  + tmp: a subfolderused to hold generated mutants.
* Subfolder throughput (This folder is used for conducting throughput experiments)
  + bench.py: By running this script, the comparison between two kinds of workflow mentioned in paper will be conducted with all files in tests
  + tests: a subfolder used to hold all IR files as input of throughput experiments.


# Expected file organization after benchmarking
* fuzzing
  + run.sh
  + tests
  + **tmp(New)**: This folder should include all mutants saved from alive-mutate with argument specified.
* throughput
  + bench.py
  + tests
  + bench1(New): a subfolder used for holding output from one workflow
  + bench2(New): a subfolder used for holding output from another workflow
  + randomSeeds.txt(New): a text file used for holding all random seeds used in the experiment
  + **res.txt(New)**: a text file with experiment results. It includes the number of files used as input, the performance improvement on each file and the average performance improvement.


# Suggsted workflow
## Fuzzing
1. Change the location of alive-mutate in run.sh if necessary
2. Execute run.sh
3. Check mutants in tmp
### Suggested customization
1. Add more files to tests folder and rerun run.sh. You can start with files in https://github.com/llvm/llvm-project/tree/main/llvm/test/Transforms/InstCombine
2. Change the command calling alive-mutate like add '--pass="instcombine"' for a different optimization pass. You can also type "alive-mutate --help" to see all arguments.
3. Change the command '-n 10' to '-n X' where X is the number of mutants you want to generate. Also, you can replace it with '-t 1'. This will make alive-mutate keeps running for 1 second.


## Throughput experiment
1. Change the location of alive-mutate in the global variable **ALIVE_PATH** if necessary
2. Execute bench.py
3. Check results in res.txt
### Suggested customization
1. Add more fiels to tests folder and rerun bench.py. For example, you can randomly select a number of files from LLVM repo. You can start with files in https://github.com/llvm/llvm-project/tree/main/llvm/test/Transforms/InstCombine
2. Change the global variable **COUNT** in bench.py. This variable controls the number of mutants generated for each input file.
3. Change the global variable **seed** in bench.py. Thisvariable controls the random seed used for the experiment.

