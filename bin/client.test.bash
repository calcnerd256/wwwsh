#!/usr/bin/env bash

curl http://localhost:8080/
curl http://localhost:8080/spawn/ -d 'cmd=ls+-al'
