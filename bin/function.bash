pushd ./"$1".function/
h.function.bash \
	| pragmaOnce.bash "$(cat ./include.macro)" \
	| cat ./emacs_variables.comment - \
	> ../"$(cat ./identifier.txt)".h \

c.function.bash < ./body.c > ../"$(cat ./identifier.txt)".c
popd

