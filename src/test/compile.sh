#!/bin/sh

## Run from src directory

threads=$(nproc)
threads=${threads:-1}

make  "-j${threads}" CC=tcc && make clean && echo "make tcc success" > ./test/compile.log || echo "make tcc failed" > ./test/compile.log
make  "-j${threads}" CC=gcc && make clean && echo "make gcc success" >> ./test/compile.log || echo "make gcc failed" >> ./test/compile.log
make  "-j${threads}" CC=clang && make clean && echo "make clang success" >> ./test/compile.log || echo "make clang failed" >> ./test/compile.log
bmake "-j${threads}" CC=tcc && bmake clean && echo "bmake tcc success" >> ./test/compile.log || echo "bmake tcc failed" >> ./test/compile.log
bmake "-j${threads}" CC=gcc && bmake clean && echo "bmake gcc success" >> ./test/compile.log || echo "bmake gcc failed" >> ./test/compile.log
bmake "-j${threads}" CC=clang && bmake clean && echo "bmake clang success" >> ./test/compile.log || echo "bmake clang failed" >> ./test/compile.log
