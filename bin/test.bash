#!/usr/bin/env bash

source ./server.test.bash
echo "valgrind=$PID_VALGRIND"
./client.test.bash
./countdown.bash
kill "$PID_VALGRIND"
sleep 1
