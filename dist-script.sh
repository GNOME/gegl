#!/bin/sh

echo hoit hoi `pwd`
cd meson-dist
echo 'Removing reference image to shrink size of tarball'
rm -v `find . | grep png | grep reference`
echo $MESON_SOURCE_ROOT $MESON_BUILD_ROOT
