#!/usr/bin/env bash

HOME=/home/tzt77/Develop/ComAir/cmake-build-debug
WORK=$(pwd)
TEST_PROGRAM=${WORK}/apache53803.c
BC_FILE=${WORK}/target.bc
PRE_BC_FILE=${WORK}/target.pre.bc
BC_ID_FILE=${WORK}/target.id.bc
BC_MARK_FILE=${WORK}/target.mark.bc
BC_PROF_FILE=${WORK}/target.aprof.bc
O_PROF_FILE=${WORK}/target.aprof.o

RUNTIME_LIB=${HOME}/runtime/AProfHooks/libAProfHooks.a

# If you want to sampling set sampling any value, or keep sampling "".
sampling=""

# sampling operation
do_sampling () {

  /HDD/llvm5.0/install/bin/clang -emit-llvm -c -g ${TEST_PROGRAM} -o ${BC_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/PrepareSampling/libPrepareSamplingPass.so -prepare-sampling  ${BC_FILE} > ${PRE_BC_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/IDAssigner/libIDAssignerPass.so -tag-id ${PRE_BC_FILE} > ${BC_ID_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/MarkFlagForAprof/libMarkFlagForAprofPass.so -mark-flags -is-sampling 1  ${BC_ID_FILE} > ${BC_MARK_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/AprofHook/libAProfHookPass.so -instrument-hooks -strFileName func_name_id.log -is-sampling 1 ${BC_MARK_FILE} > ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llvm-dis  ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llc -filetype=obj ${BC_PROF_FILE} -o ${O_PROF_FILE}

## # link runtime lib
 /HDD/llvm5.0/install/bin/clang -O2  ${O_PROF_FILE} ${RUNTIME_LIB} -lm -lrt

 ./a.out input.txt song

 # run some times and calculate result
 # python run_apache34464.py

}

do_not_sampling () {

 /HDD/llvm5.0/install/bin/clang -emit-llvm -c -g ${TEST_PROGRAM} -o ${BC_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/IDAssigner/libIDAssignerPass.so -tag-id ${BC_FILE} > ${BC_ID_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/MarkFlagForAprof/libMarkFlagForAprofPass.so -mark-flags  ${BC_ID_FILE} > ${BC_MARK_FILE}

 /HDD/llvm5.0/install/bin/opt -load ${HOME}/lib/AprofHook/libAProfHookPass.so -instrument-hooks -strFileName func_name_id.log ${BC_MARK_FILE} > ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llvm-dis  ${BC_PROF_FILE}

 /HDD/llvm5.0/install/bin/llc -filetype=obj ${BC_PROF_FILE} -o ${O_PROF_FILE}

## # link runtime lib
 /HDD/llvm5.0/install/bin/clang -O2  ${O_PROF_FILE} ${RUNTIME_LIB} -lm -lrt

 ./a.out 1000

 rm *.bc *o *.ll

 # run some times and calculate result
 # python run_apache34464.py

}

run_command() {

  if [ "$sampling" ]; then
    do_sampling
  else
   do_not_sampling
  fi

}

# call run command.
run_command
