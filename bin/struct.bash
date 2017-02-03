#!/usr/bin/env bash

INCLUDE_MACRO="INCLUDE_$(cat ./include.macro)_STRUCT"
STRUCT_IDENTIFIER="$(cat ./identifier.txt)"

cat ./emacs_variables.comment
echo
echo "#ifndef $INCLUDE_MACRO"
echo "#define $INCLUDE_MACRO"
echo
echo "struct $STRUCT_IDENTIFIER{"
for FIELD_NAME in $(cat ./fields); do
	FIELD_TYPE="$(cat "$FIELD_NAME".field)"
	FIELD_DECL="$(echo "$FIELD_TYPE $FIELD_NAME" | sed "s/ \* / */")"
	echo -e "\t$FIELD_DECL;"
done
echo "};"
echo
echo "#endif"
