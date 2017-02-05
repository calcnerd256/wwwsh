#!/usr/bin/env bash

sed "s/^/#include \".\//;s/\$/.h\"/" ./header.dependencies.list
echo
function_header.bash \
	"$(cat ./identifier.txt)" \
	"$(cat ./return.type)" \
	< ./argument.list \
	| sed "\$s/\$/;/" \
