#!/usr/bin/env bash

valgrind -v ../build/main 8080 &
export PID_VALGRIND="$!"
