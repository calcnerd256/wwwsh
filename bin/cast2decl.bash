#!/usr/bin/env bash

IN_EXPR_REGEX="$1"
OUT_EXPR="$2"

bash -c "echo -n \"explain \"; cat -" \
 | cdecl \
 | sed "s/cast $IN_EXPR_REGEX into/declare $OUT_EXPR as/" \
 | cdecl \

