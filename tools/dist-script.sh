#!/bin/sh

echo hoit hoi `pwd`
cd meson-dist
echo 'Removing reference image to shrink size of tarball'
rm -v `find . | grep png | grep reference`
echo $MESON_SOURCE_ROOT $MESON_BUILD_ROOT

# copy docs into distro
echo 'Copying README and NEWS to distribution'
cp $MESON_BUILD_ROOT/README $MESON_DIST_ROOT
cp $MESON_BUILD_ROOT/NEWS $MESON_DIST_ROOT
