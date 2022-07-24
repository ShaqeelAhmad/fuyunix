#!/bin/sh

## Run from src directory

compile() {
	Make="$1"
	CC="$2"

	"$Make" clean 2>&1 >&-
	if "$Make" CC="$CC" 2>&1 >&-
	then
		printf "%s %s success\n" "$Make" "$CC"
	else
		printf "%s %s failed\n" "$Make" "$CC"
	fi
}

compile make tcc
compile make gcc
compile make clang
compile bmake tcc
compile bmake gcc
compile bmake clang
