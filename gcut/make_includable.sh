#!/usr/bin/env bash

input="$1"

delim="FOR_C_INCLUDE"

echo "R\"${delim}(\n"
cat "${input}"
echo ")${delim}\""
