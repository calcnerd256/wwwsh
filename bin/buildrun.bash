#!/usr/bin/env bash

time ./build.bash || exit 1
time ./test.bash
