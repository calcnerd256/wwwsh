#!/usr/bin/env bash

cat ./emacs_variables.comment
DEPEND_FILES=""
if [ -a ./header.dependencies.list ] ; then
	DEPEND_FILES="$DEPEND_FILES ./header.dependencies.list"
fi
if [ -a ./body.dependencies.list ] ; then
	DEPEND_FILES="$DEPEND_FILES ./body.dependencies.list"
fi
echo "$1" | cat - $DEPEND_FILES | sed "s/^/#include \".\//;s/\$/.h\"/"
echo
function_header.bash \
	"$(cat ./identifier.txt)" \
	"$(cat ./return.type)" \
	< ./argument.list \
	| tr '\n\t' '  ' \
	| sed "s/  */ /g" \
	| sed "s/( /(/;s/ *) */)/;s/ \* / */g" \

echo "{"
sed "s/^/\t/"
echo "}"
