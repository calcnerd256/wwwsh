#!/usr/bin/env bash

cat ./emacs_variables.comment
echo "#include \"./$(cat ./identifier.txt).h\""
sed "s/^/#include \".\//;s/\$/.h\"/" ./header.dependencies.list
sed "s/^/#include \".\//;s/\$/.h\"/" ./body.dependencies.list
echo
echo $(function_header.bash "$(cat ./identifier.txt)" "$(cat ./return.type)" < ./argument.list)"{" | sed "s/( /(/;s/ )/)/"
sed "s/^/\t/"
echo "}"
