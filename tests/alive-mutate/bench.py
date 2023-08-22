import os
import time
import shutil
import random
import filecmp

TEST_FILES_DIR = "./tests/"
TMP_FILES_DIR = "."
COUNT=10
#would be read from a fixed external file
#seed=random.randint(0, (1<<31)-1)
seed=0
seeds=[]
seeds_file="randomSeeds.txt"
GEN_TESTS_DIR = "./tests/"
TMP_DIRS = ["./bench1/", "./bench2/"]

RANDOM_SEEDS_COMMAND="~/GitRepo/alive2/build/RNG -n {count} -s {seed} > {seeds_file}"

ALIVE_MUTATE_COMMAND = ("~/GitRepo/alive2/build/alive-mutate {input} {dir} -n {count} "
                  "--disable-undef-input --disable-poison-input --removeUndef"
                        " --randomMutate --masterRNG -s {seed}")

ALIVE_MUTATE_MASTER_NON_VERIFY_COMMAND = ("~/GitRepo/alive2/build/alive-mutate {input} {dir} -n {count} "
                  "--disable-undef-input --disable-poison-input --removeUndef"
                        " --randomMutate --masterRNG --disableAlive -s {seed}")

ALIVE_MUTATE_NON_VERIFY = ("~/GitRepo/alive2/build/alive-mutate {input} {dir} -n {count} "
                  "--disableAlive --removeUndef --randomMutate -s {seed}")

ALIVE_TV_COMMAND=("~/GitRepo/alive2/build/alive-tv {input} --quiet --disable-undef-input --disable-poison-input "
                  "--src-fn={func}")

def getOutputFilename(input,ith):
    filename,extension=os.path.splitext(input)
    return TMP_DIRS[1]+filename+str(ith)+extension

def getInputFilename(input):
    return TEST_FILES_DIR+input

def getTargetFunction(output):
    with open(output) as f:
        str=f.readline()
        return str[1:]


def getRandomSeed():
    global seeds
    if os.path.exists(seeds_file):
        os.remove(seeds_file)
    GEN_command=RANDOM_SEEDS_COMMAND.format(count=COUNT,seed=seed,seeds_file=seeds_file)
    os.system(GEN_command)
    with open(seeds_file) as f:
        s=f.readline()
        seeds=[tmp for tmp in s.split(" ") if len(tmp) > 0]
    print(seeds)

def make_tmp_dirs():
    global TMP_FILES_DIR
    assert os.path.isdir(TMP_FILES_DIR)
    if TMP_FILES_DIR[-1] != '/':
        TMP_FILES_DIR += '/'
    for i, dir in enumerate(TMP_DIRS):
        dir_path = TMP_FILES_DIR + dir
        TMP_DIRS[i] = dir_path
        if os.path.isdir(dir_path):
            shutil.rmtree(dir_path)
        os.mkdir(dir_path)


def bench1(input):
    alive_mutate=ALIVE_MUTATE_COMMAND.format(input=TEST_FILES_DIR+input,dir=TMP_DIRS[0], count=COUNT, seed=seed)
    print(alive_mutate)
    start = time.time()
    os.system(alive_mutate)
    end=time.time()
    return end-start


def bench2(input):
    result=0
    for i in range(COUNT-1,-1,-1):
        alive_non_verify=ALIVE_MUTATE_NON_VERIFY.format(input=TEST_FILES_DIR+input, dir=TMP_DIRS[1], count=1, seed=seeds[i])
        print(alive_non_verify)
        start=time.time()
        os.system(alive_non_verify)
        end=time.time()
        result+=end-start
        #execute alive-mutate
        outputFile=getOutputFilename(input,0)
        currentFunction=getTargetFunction(outputFile)
        alive_tv=ALIVE_TV_COMMAND.format(input=outputFile, func=currentFunction)
        #print(alive_tv)
        start = time.time()
        # execute alive-tv
        os.system(alive_tv)
        #print(alive_tv)
        end=time.time()
        os.rename(outputFile,getOutputFilename(input,i))
        result+=end-start
    return result

def validityCheck(input):
    alive_mutate = ALIVE_MUTATE_MASTER_NON_VERIFY_COMMAND.format(input=input, dir=TMP_DIRS[0], count=COUNT, seed=seed)
    os.system(alive_mutate)
    result=True
    for file in os.listdir(TMP_DIRS[1]):
        result=filecmp.cmp(TMP_DIRS[0]+file, TMP_DIRS[1]+file)
        if not result:
            return False
    return True


def performExperiment():
    make_tmp_dirs()
    getRandomSeed()
    global TEST_FILES_DIR
    if TEST_FILES_DIR[-1]!='/':
        TEST_FILES_DIR+='/'
    bench1Sum=0
    bench2Sum=0
    total=0
    invalidLst=[]
    for i,file in enumerate(os.listdir(TEST_FILES_DIR)):
        whole_file=TEST_FILES_DIR+file
        if os.path.isfile(whole_file) and whole_file.endswith(".ll"):
            bench2Res = bench2(file)
            print("bench2 ", bench2Res)
            bench1Res=bench1(file)
            print("bench1 ",bench1Res)
            validRes=validityCheck(whole_file)
            if validRes:
                total+=1
                bench1Sum+=bench1Res
                bench2Sum+=bench2Res
            else:
                invalidLst.append(lst)

    print("Total: ", total)
    print("Bench1 Sum:", bench1Sum)
    print("Bench2 Sum:", bench2Sum)
    print("Invalid files:", invalidLst)



#bench1()
#bench2()
performExperiment()

#getRandomSeed()