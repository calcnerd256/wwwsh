#!/usr/bin/env bash

curl http://localhost:8080/
curl http://localhost:8080/index/
KIDPID=$(curl http://localhost:8080/spawn/ -d cmd=ls+-al | grep href | sed "s-.*/process/--" | sed "s-/.*--")
curl http://localhost:8080/index/
curl "http://localhost:8080/process/$KIDPID/"
curl http://localhost:8080/index/
curl "http://localhost:8080/process/$KIDPID/delete" -d "confirm=checked"
curl http://localhost:8080/index/
