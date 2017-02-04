#!/usr/bin/env bash

STRUCT_IDENTIFIER="$1"

sed "s/^/#include \".\//;s/\$/.h\"/" ./dependencies.list
echo
echo "struct $STRUCT_IDENTIFIER{"
for FIELD_NAME in $(cat -); do
	if which cdecl > /dev/null ; then
		echo "($(cat "$FIELD_NAME".field))_" \
			| cast2decl.bash _ "$FIELD_NAME" \
			| sed "s/^/\t/;s/\$/;/" \
			
	else
		# Unfortunately, I can't afford to depend upon cdecl.
		# So, instead, I'll have to splice together a prefix and a suffix.
		# I can key off of the file extension to allow the simple case.
		# Unfortunately, if I want to also support the cdecl case,
		#  then I need to require complex cases to choose
		#  whether to support both.
		FIELD_DECL_PREFIX=""
		FIELD_DECL_SUFFIX=""
		if [ -a "$FIELD_NAME".field.prefix ] ; then
			FIELD_DECL_PREFIX="$(cat "$FIELD_NAME".field.prefix)"
			FIELD_DECL_SUFFIX="$(cat "$FIELD_NAME".field.suffix)"
		else
			FIELD_DECL_PREFIX="$(cat "$FIELD_NAME".field)"
			FIELD_DECL_SUFFIX=""
		fi
		echo -ne "\t"
		echo "$FIELD_DECL_PREFIX $FIELD_NAME $FIELD_DECL_SUFFIX;" \
			| sed "s/ \* / */" \

	fi
done
echo "};"
