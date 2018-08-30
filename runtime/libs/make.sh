#!/usr/bin/env bash

cd ./AProfHooks && make clean &&  make

cd ../AProfCounterHooks && make clean &&  make

cd ../AProfDumpCallStack/ && make clean &&  make

cd ../AProfLoopArrayListHooks/ && make clean &&  make

cd ../AProfLoopSampleHooks/ && make clean &&  make

cd  ../AProfRecursiveHooks/ && make clean &&  make

cd  ../AProfRecursiveSampleHooks/ && make clean &&  make

echo "Done!"
