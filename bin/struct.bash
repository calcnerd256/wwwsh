#!/usr/bin/env bash

INCLUDE_MACRO="INCLUDE_$(cat ./include.macro)_STRUCT"
STRUCT_IDENTIFIER="$(cat ./identifier.txt)"

cat ./emacs_variables.comment
echo
echo "#ifndef $INCLUDE_MACRO"
echo "#define $INCLUDE_MACRO"
echo
sed "s/^/#include \".\//;s/\$/.h\"/" ./dependencies.list
echo
echo "struct $STRUCT_IDENTIFIER{"
for FIELD_NAME in $(cat ./fields); do
	FIELD_TYPE="$(cat "$FIELD_NAME".field)"
	FIELD_DECL="$(echo "explain ($FIELD_TYPE)_" | cdecl | sed "s/cast _ into/declare $FIELD_NAME as/" | cdecl)"
	echo -e "\t$FIELD_DECL;"
done
echo "};"
echo
echo "#endif"
