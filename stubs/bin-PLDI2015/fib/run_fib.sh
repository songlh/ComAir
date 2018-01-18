#!/usr/bin/env bash

HOME=/home/tzt77/Develop/ComAir/cmake-build-debug
WORK=/home/tzt77/Develop/ComAir/stubs/bin-PLDI2015/fib
TEST_PROGRAM=${WORK}/fib-base.cpp
BC_FILE=${WORK}/test.bc
BC_ID_FILE=${WORK}/test.id.bc
BC_MARK_FILE=${WORK}/test.mark.bc
BC_PROF_FILE=${WORK}/test.aprof.bc
O_PROF_FILE=${WORK}/test.aprof.o

RUNTIME_LIB=${HOME}/runtime/AProfHooks/libAProfHooks.a


{

 /HDD/llvm5.0/install/bin/clang++ -emit-llvm -c -g ${TEST_PROGRAM} -o ${BC_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/IDAssigner/libIDAssignerPass.so -tag-id ${BC_FILE} > ${BC_ID_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/MarkFlagForAprof/libMarkFlagForAprofPass.so -mark-flag-aprof ${BC_ID_FILE} > ${BC_MARK_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/AprofHook/libAProfHookPass.so -algo-profiling ${BC_MARK_FILE} > ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llvm-dis  ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llc -filetype=obj ${BC_PROF_FILE} -o ${O_PROF_FILE}

 # link c++
 g++ ${O_PROF_FILE} ${RUNTIME_LIB} -lstdc++ -lrt

 # test one time
 ./a.out 3

 # run some times and calculate result
 # python run_fib.py

}