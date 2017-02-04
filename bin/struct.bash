#!/usr/bin/env bash

STRUCT_IDENTIFIER="$1"

sed "s/^/#include \".\//;s/\$/.h\"/" ./dependencies.list
echo
echo "struct $STRUCT_IDENTIFIER{"
for FIELD_NAME in $(cat ./fields); do
	FIELD_TYPE="$(cat "$FIELD_NAME".field)"
	FIELD_DECL="$(echo "explain ($FIELD_TYPE)_" | cdecl | sed "s/cast _ into/declare $FIELD_NAME as/" | cdecl)"
	echo -e "\t$FIELD_DECL;"
done
echo "};"
