#!/usr/bin/env bash

cd ./AProfHooks && make clean &&  make

cd ../AProfLoopArrayListHooks/ && make clean &&  make

cd ../AProfLoopSampleHooks/ && make clean &&  make

cd  ../AProfRecursiveHooks/ && make clean &&  make

echo "Done!"
