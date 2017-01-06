#!/usr/bin/env bash

valgrind -v ../build/main 8080 &
PID_VALGRIND="$!"
sleep 1
echo "valgrind=$PID_VALGRIND"
curl http://localhost:8080/
curl http://localhost:8080/formtest/ -d cmd=ls+-al
sleep 2
kill "$PID_VALGRIND"
sleep 1
