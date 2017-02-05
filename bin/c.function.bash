#!/usr/bin/env bash

cat ./emacs_variables.comment
DEPEND_FILES="./identifier.txt"
if [ -a ./header.dependencies.list ] ; then
	DEPEND_FILES="$DEPEND_FILES ./header.dependencies.list"
fi
if [ -a ./body.dependencies.list ] ; then
	DEPEND_FILES="$DEPEND_FILES ./body.dependencies.list"
fi
cat $DEPEND_FILES | sed "s/^/#include \".\//;s/\$/.h\"/"
echo
function_header.bash \
	"$(cat ./identifier.txt)" \
	"$(cat ./return.type)" \
	< ./argument.list \
	| sed "s/( /(/;s/ )/)/;\$s/\$/{/" \

sed "s/^/\t/"
echo "}"
