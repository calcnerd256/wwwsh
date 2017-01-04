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
 ../src/lines.c \
 ../src/chunkStream.c \
 ../src/requestInput.c \
 ../src/network.c \
 ../src/server.c \
 ../src/request.c \
 ../src/static.c \
 ../src/main.c
