#!/usr/bin/env bash

time ./build.bash || exit 1
valgrind -v ../build/main 8080 &
PID_VALGRIND="$!"
sleep 1
curl localhost:8080 &
PID_CURL="$!"
echo "valgrind=$PID_VALGRIND"
echo "curl=$PID_CURL"
sleep 1
./countdown.bash
kill "$PID_CURL"
sleep 1
sleep 4
./countdown.bash
kill "$PID_VALGRIND"
