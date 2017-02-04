#!/usr/bin/env bash

TYPE_IDENTIFIER="$1"
RETURN_TYPE="$(cat ./return.type)"

sed "s/^/#include \".\//;s/\$/.h\"/" ./dependencies.list
echo
echo "typedef" "$RETURN_TYPE" "${TYPE_IDENTIFIER}("
sed "s/^/\t/;s/\$/,/" ./argument.type.list | sed "\$s/,\$//"
echo ");"
