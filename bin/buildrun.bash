#!/usr/bin/env bash

time ./build.bash || exit 1
valgrind -v ../build/main 8080 &
PID_VALGRIND="$!"
sleep 1
echo "valgrind=$PID_VALGRIND"
curl localhost:8080
sleep 1
kill "$PID_VALGRIND"
sleep 1
