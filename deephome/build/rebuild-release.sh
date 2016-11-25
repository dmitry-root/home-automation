#!/bin/bash

if [[ -r Makefile ]]; then
	make clean
fi

rm -f Makefile CMakeCache.txt

cmake -D CMAKE_BUILD_TYPE=Release ..
make

