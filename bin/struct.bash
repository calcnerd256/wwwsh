#!/usr/bin/env bash

STRUCT_IDENTIFIER="$1"

sed "s/^/#include \".\//;s/\$/.h\"/" ./dependencies.list
echo
echo "struct $STRUCT_IDENTIFIER{"
for FIELD_NAME in $(cat -); do
	declare.bash "$FIELD_NAME" | sed "s/^/\t/;\$s/\$/;/"
done
echo "};"
