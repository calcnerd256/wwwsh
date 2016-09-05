#!/usr/bin/env bash

./build.bash
valgrind ../build/main 8080 & (
	sleep 1; curl localhost:8080
)
