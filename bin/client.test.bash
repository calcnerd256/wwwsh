#!/usr/bin/env bash

curl http://localhost:8080/
curl http://localhost:8080/formtest/ -d 'test=on&cmd=ls+-al'
