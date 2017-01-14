#!/usr/bin/env bash

curl http://localhost:8080/
KIDPID=$(curl http://localhost:8080/spawn/ -d cmd=ls+-al | grep href | sed "s-.*/process/--" | sed "s-/.*--")
curl "http://localhost:8080/process/$KIDPID/"
