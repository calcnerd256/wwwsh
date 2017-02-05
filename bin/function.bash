pushd ./"$1".function/

EXPECTATIONS="include.macro emacs_variables.comment identifier.txt return.type argument.list body.dependencies.list body.c"
for EXPECTATION in $EXPECTATIONS ; do
	if ! [ -a "./$EXPECTATION" ] ; then
		echo "missing file: $(pwd)/$EXPECTATION"
		exit 1
	fi
done

h.function.bash \
	| pragmaOnce.bash "$(cat ./include.macro)" \
	| cat ./emacs_variables.comment - \
	> ../"$(cat ./identifier.txt)".h \

c.function.bash < ./body.c > ../"$(cat ./identifier.txt)".c
popd

