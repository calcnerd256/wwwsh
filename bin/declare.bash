#!/usr/bin/env bash

FIELD_NAME="$1"

if which cdecl > /dev/null ; then
	echo "($(cat "$FIELD_NAME".field))_" \
		| cast2decl.bash _ "$FIELD_NAME" \

	exit 0
fi

# else

if [ -a "$FIELD_NAME".field.prefix ] ; then
	echo " $FIELD_NAME " \
		| cat \
			"$FIELD_NAME".field.prefix \
			- \
			"$FIELD_NAME".field.suffix \
		| sed "s/ \* / */" \

	exit 0
fi

# else

echo "$FIELD_NAME" \
	| cat \
		"$FIELD_NAME".field \
		- \
	| sed "s/ \* / */"
