#!/usr/bin/env bash

IDENTIFIER="$1"
RETURN_TYPE="$2"

echo "$RETURN_TYPE" "${IDENTIFIER}(" | sed "s/ \* / */"
ARGUMENT_NAMES=($(cat -))
ARG_LEN=${#ARGUMENT_NAMES[*]}
i=0
while [ $i -lt $ARG_LEN ]; do
	ARGUMENT_NAME="${ARGUMENT_NAMES[$i]}"
	let i++
	COMMA=""
	if [ $i -lt $ARG_LEN ] ; then
		COMMA=";\$s/\$/,/"
	fi
	declare.bash "$ARGUMENT_NAME" | sed "s/^/\t/$COMMA"
done
echo ")"
