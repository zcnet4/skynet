#!/bin/sh

find . -name "*.sh"|xargs chmod 755 && ./autogen.sh --with-jemalloc-prefix=je_ --disable-valgrind
make CC=gcc