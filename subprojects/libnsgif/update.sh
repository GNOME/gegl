#!/bin/bash

UPSTREAM="https://source.netsurf-browser.org/libnsgif.git"
LOCAL="temp"

git clone "$UPSTREAM" "$LOCAL"

echo "Copying sources"
cp "$LOCAL"/COPYING .
cp "$LOCAL"/README.md .
cp "$LOCAL"/include/nsgif.h .
cp "$LOCAL"/src/gif.c .
cp "$LOCAL"/src/lzw.c .
cp "$LOCAL"/src/lzw.h .

echo "Upstream: $UPSTREAM"
echo "Ref: `git -C "$LOCAL" rev-parse HEAD`"

rm -rf "$LOCAL"
