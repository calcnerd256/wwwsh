#!/usr/bin/env bash

FIELD_NAME="$1"

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

FILE="$FIELD_NAME"
if [ -a "$FIELD_NAME".field ] ; then
	FILE="$FIELD_NAME".field
else
	if [ -a "$FIELD_NAME".argument ] ; then
		FILE="$FIELD_NAME".argument
	fi
fi

echo "$(cat "$FILE")" "$FIELD_NAME" \
	| sed "s/ \* / */"
