#!/usr/bin/env bash

if [ -a ./header.dependencies.list ] ; then
	sed "s/^/#include \".\//;s/\$/.h\"/" ./header.dependencies.list
	echo
fi
function_header.bash \
	"$(cat ./identifier.txt)" \
	"$(cat ./return.type)" \
	< ./argument.list \
	| sed "\$s/\$/;/" \
