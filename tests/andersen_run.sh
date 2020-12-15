PROG=$1
clang -O0 -emit-llvm $PROG.c -c -o ${PROG}_pre.bc
opt -load ~/583_final/andersen/build/lib/libAndersen.so -aa-eval -anders-aa < ${PROG}_pre.bc > $PROG.bc
# llvm-dis < $PROG.bc | less
# llc $PROG.bc -o $PROG.s
# gcc -no-pie $PROG.s -o $PROG.native
# ./$PROG.native 
