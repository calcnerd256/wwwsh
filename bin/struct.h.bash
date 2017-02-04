#!/usr/bin/env bash

pushd "$1"

struct.bash "$(cat ./identifier.txt)" \
	< ./fields \
	| pragmaOnce.bash "$(cat ./include.macro)_STRUCT" \
	| cat ./emacs_variables.comment - \
	> ../"$2".h \

popd
