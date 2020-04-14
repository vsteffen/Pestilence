#!/bin/bash

C_STR=

if [ $# -eq 0 ]; then
	read C_STR
else
	C_STR="$1"
fi

echo -n '(char []){'
echo -n "$C_STR" | sed -e 's/\(.\)/\'"'"'\1\'"'"',/g'
echo "'\\0'}"
