import os
import time
import shutil
import random

TEST_FILES_DIR = "."
TMP_FILES_DIR = "."
COUNT=100
#would be read from a fixed external file
seeds=[]
seeds=[i for i in range(10)]
seeds_file="randomSeeds.txt"
GEN_TESTS_DIR = "./tests/"
TMP_DIRS = ["./bench1/", "./bench2/"]

ALIVE_MUTATE_COMMAND = ("~/GitRepo/alive2/build/alive-mutate {input} {dir} -n {count} "
                  "--disable-undef-input --disable-poison-input --removeUndef -s {seed}")

ALIVE_MUTATE_NON_VERIFY = ("~/GitRepo/alive2/build/alive-mutate {input} {dir} -n {count} "
                  "--disableAlive --removeUndef -s {seed}")

ALIVE_IDEN = ("~/GitRepo/alive2/build/alive-iden {input} {output} ")

ALIVE_TV_COMMAND=("~/GitRepo/alive2/build/alive-tv {input} --quiet --disable-undef-input --disable-poison-input "
                  "--src-fn={func}")

def getOutputFilename(input,ith):
    filename,extension=os.path.splitext(input)
    return TMP_DIRS[1]+filename+str(ith)+extension


def getTargetFunction(output):
    with open(output) as f:
        str=f.readline()
        return str[1:]


def genRandomSeed():
    global seeds
    if os.path.exists(seeds_file):
        with open(seeds_file) as fin:
            seeds.append(fin.read())
    else:
        with open(seeds_file) as fout:
            for i in range(COUNT):
                seeds.append(random.randint())
                fout.write(seeds[-1])
                fout.write(" ")

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


def genTests(input, seed):
    if os.path.isdir(GEN_TESTS_DIR):
        shutil.rmtree(GEN_TESTS_DIR)
    os.mkdir(GEN_TESTS_DIR)
    alive_non_verify = ALIVE_MUTATE_NON_VERIFY.format(input=input, dir=GEN_TESTS_DIR, count=COUNT, seed=seed)
    os.system(alive_non_verify)


def bench1(input, seed):
    alive_mutate=ALIVE_MUTATE_COMMAND.format(input=input,dir=TMP_DIRS[0], count=COUNT, seed=seed)
    print(alive_mutate)
    start = time.time()
    os.system(alive_mutate)
    end=time.time()
    return end-start


def bench2(input, seed):
    result=0
    genTests(input,seed)
    for file in os.listdir(GEN_TESTS_DIR):
        whole_file=GEN_TESTS_DIR+file
        alive_non_verify=ALIVE_IDEN.format(input=whole_file, output=TMP_DIRS[1])
        #print(alive_non_verify)
        start=time.time()
        os.system(alive_non_verify)
        #print(alive_non_verify)
        end=time.time()
        result+=end-start
        #execute alive-mutate
        outputFile=getOutputFilename(file,0)
        currentFunction=getTargetFunction(whole_file)
        alive_tv=ALIVE_TV_COMMAND.format(input=outputFile, func=currentFunction)
        #print(alive_tv)
        start = time.time()
        # execute alive-tv
        os.system(alive_tv)
        #print(alive_tv)
        end=time.time()
        result+=end-start
    return result;

def performExperiment():
    make_tmp_dirs()
    global TEST_FILES_DIR
    if TEST_FILES_DIR[-1]!='/':
        TEST_FILES_DIR+='/'
    for i,file in enumerate(os.listdir(TEST_FILES_DIR)):
        whole_file=TEST_FILES_DIR+file
        if os.path.isfile(file) and whole_file.endswith(".ll"):
            print(whole_file)
            bench2Res = bench2(whole_file, seeds[i])
            print("bench2 ", bench2Res)
            bench1Res=bench1(whole_file,seeds[i])
            print("bench1 ",bench1Res)

#bench1()
#bench2()
performExperiment()