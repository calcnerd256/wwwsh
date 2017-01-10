#!/usr/bin/env bash

./delay.bash &
PID_DELAY="$!"
kill -s SIGSTOP "$PID_DELAY"

valgrind -v --leak-check=full --show-leak-kinds=all \
 ../build/main 8080 "$PID_DELAY" &

export PID_VALGRIND="$!"
wait "$PID_DELAY"
