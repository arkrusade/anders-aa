#!/bin/bash
PROG=cat
clang -O0 -emit-llvm src/$PROG.c -c -o build/$PROG.bc
opt -load ../build/mypass/LLVMPJT.so -mypass < build/$PROG.bc > /dev/null
