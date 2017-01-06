#!/usr/bin/env bash

source ./server.test.bash
sleep 1
echo "valgrind=$PID_VALGRIND"
./client.test.bash
sleep 2
kill "$PID_VALGRIND"
sleep 1
