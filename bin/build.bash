#!/usr/bin/env bash

mkdir -p ../build/
gcc \
 -g3 \
 -O3 \
 -Wall \
 -Wpedantic \
 -Wextra \
 -Werror \
 -o ../build/main \
 ../src/linked_list.c \
 ../src/mempool.c \
 ../src/requestInput.c \
 ../src/network.c \
 ../src/server.c \
 ../src/request.c \
 ../src/static.c \
 ../src/form.c \
 ../src/main.c
