#!/usr/bin/env bash

./build.bash
../build/main 8080 & curl localhost:8080
