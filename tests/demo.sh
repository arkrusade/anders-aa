PROG=$1
clang -O0 -emit-llvm $PROG.c -c -o ${PROG}_pre.bc
opt -load ~/583_final/andersen/build/lib/libAndersen.so -aa-eval -anders-aa < ${PROG}_pre.bc > $PROG.bc


opt -aa-eval -cfl-anders-aa < ${PROG}_pre.bc > $PROG.bc