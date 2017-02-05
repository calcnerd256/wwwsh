#!/usr/bin/env bash

DIR_DEST=./"$1".function
mkdir "$DIR_DEST"
cp ./*/emacs_variables.comment "$DIR_DEST" &> /dev/null

echo "$1" | sed "s/_a/A/;s/_b/B/;s/_c/C/;s/_d/D/;s/_e/E/;s/_f/F/;s/_g/G/;s/_h/H/;s/_i/I/;s/_j/J/;s/_k/K/;s/_l/L/;s/_m/M/;s/_n/N/;s/_o/O/;s/_p/P/;s/_q/Q/;s/_r/R/;s/_s/S/;s/_t/T/;s/_u/U/;s/_v/V/;s/_w/W/;s/_x/X/;s/_y/Y/;s/_z/Z/" > "$DIR_DEST"/include.macro
echo "$1" > "$DIR_DEST"/identifier.txt
echo "$2" > "$DIR_DEST"/return.type
echo "optional: $DIR_DEST/header.dependencies.list"
touch "$DIR_DEST/argument.list"
echo "mandatory: $DIR_DEST/argument.list"
echo "optional: $DIR_DEST/body.dependencies.list"
touch "$DIR_DEST/body.c"
echo "mandatory: $DIR_DEST/body.c"
