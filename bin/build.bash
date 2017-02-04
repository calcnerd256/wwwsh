#!/usr/bin/env bash

DIR_ROOT=..
DIR_BUILD="$DIR_ROOT"/build
DIR_SRC="$DIR_ROOT"/src

GCC_OPTIMIZATION=""
GCC_OPTIMIZATION="$GCC_OPTIMIZATION -g3"
GCC_OPTIMIZATION="$GCC_OPTIMIZATION -O3"

GCC_WARNINGS=""
GCC_WARNINGS="$GCC_WARNINGS -Wall"
GCC_WARNINGS="$GCC_WARNINGS -Wpedantic"
GCC_WARNINGS="$GCC_WARNINGS -Wextra"
GCC_WARNINGS="$GCC_WARNINGS -Werror"

GCC_OUT="$DIR_BUILD/main"

GCC_UNITS=""
GCC_UNITS="$GCC_UNITS $DIR_SRC/linkedList/linkedList.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/mempool.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/requestInput.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/network.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/server.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/request.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/static.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/form.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/process.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/event.c"
GCC_UNITS="$GCC_UNITS $DIR_SRC/main.c"

GCC_FLAGS=""
GCC_FLAGS="$GCC_FLAGS $GCC_OPTIMIZATION"
GCC_FLAGS="$GCC_FLAGS $GCC_WARNINGS"

DIR_BIN="$(pwd)"
pushd "$DIR_SRC"/linkedList/linkedList.struct/
"$DIR_BIN"/struct.bash "$(cat ./identifier.txt)" \
 < ./fields \
 | "$DIR_BIN"/pragmaOnce.bash "$(cat ./include.macro)_STRUCT" \
 | cat ./emacs_variables.comment - \
 > ../linkedList.struct.h
cd ../dequoid.struct/
"$DIR_BIN"/struct.bash "$(cat ./identifier.txt)" \
 < ./fields \
 | "$DIR_BIN"/pragmaOnce.bash "$(cat ./include.macro)_STRUCT" \
 | cat ./emacs_variables.comment - \
 > ../dequoid.struct.h
popd

mkdir -p "$DIR_BUILD"
gcc $GCC_FLAGS -o "$GCC_OUT" $GCC_UNITS
