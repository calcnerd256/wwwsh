#!/usr/bin/env bash

./server.test.bash &
PID_VALGRIND="$!"
sleep 1
echo "valgrind=$PID_VALGRIND"
./client.test.bash
sleep 2
kill "$PID_VALGRIND"
sleep 1
