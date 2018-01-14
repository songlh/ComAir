#!/usr/bin/env bash

HOME=/home/tzt77/Develop/ComAir/cmake-build-debug
WORK=/home/tzt77/Develop/ComAir/stubs/apache34464
TEST_PROGRAM=/home/tzt77/Develop/ComAir/stubs/apache34464/Telnet.c
BC_FILE=${WORK}/Telnet.bc
BC_ID_FILE=${WORK}/Telnet.id.bc
BC_PROF_FILE=${WORK}/Telnet.aprof.bc
O_PROF_FILE=${WORK}/Telnet.aprof.o

RUNTIME_LIB=${HOME}/runtime/AProfHooks/libAProfHooks.a


{

 /HDD/llvm5.0/install/bin/clang -emit-llvm -c -g ${TEST_PROGRAM} -o ${BC_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/IDAssigner/libIDAssignerPass.so -tag-id ${BC_FILE} > ${BC_ID_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/AprofHook/libAProfHookPass.so -algo-profiling ${BC_ID_FILE} > ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llvm-dis  ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llc -filetype=obj ${BC_PROF_FILE} -o ${O_PROF_FILE}

 # link runtime lib
 gcc Telnet.aprof.o ${RUNTIME_LIB} -lstdc++

 ./a.out input.txt song
}