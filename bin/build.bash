#!/usr/bin/env bash

mkdir -p ../build/
gcc \
 -g3 \
 -O3 \
 -Wall \
 -Wpedantic \
 -Wextra \
 -o ../build/main \
 ../src/linked_list.c \
 ../src/mempool.c \
 ../src/lines.c \
 ../src/main.c
