#!/usr/bin/env bash

INCLUDE_MACRO="$1"

echo "#ifndef INCLUDE_$INCLUDE_MACRO"
echo "#define INCLUDE_$INCLUDE_MACRO"
echo
sed "s/^#/& /"
echo
echo "#endif"
