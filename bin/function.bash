pushd ./"$1".function/

EXPECTATIONS="include.macro emacs_variables.comment identifier.txt return.type argument.list body.c"
for EXPECTATION in $EXPECTATIONS ; do
	if ! [ -a "./$EXPECTATION" ] ; then
		echo "missing file: $(pwd)/$EXPECTATION"
		exit 1
	fi
done
DEPEND_FILES=""
if [ -a ./header.dependencies.list ] ; then
	DEPEND_FILES="$ARGS_FILES ./header.dependencies.list"
fi
if [ -a ./body.dependencies.list ] ; then
	DEPEND_FILES="$ARGS_FILES ./body.dependencies.list"
fi

for ARGUMENT in $(cat ./argument.list) ; do
	if ! [ -a ./"$ARGUMENT".argument ] ; then
		echo "missing file: $(pwd)/$ARGUMENT.argument"
		exit 1
	fi
done

h.function.bash \
	| pragmaOnce.bash "$(cat ./include.macro)" \
	| cat ./emacs_variables.comment - \
	> ../"$(cat ./identifier.txt)".h \

c.function.bash < ./body.c > ../"$(cat ./identifier.txt)".c
popd
