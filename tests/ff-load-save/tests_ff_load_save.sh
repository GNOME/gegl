#!/usr/bin/env bash

export GEGL_PATH="${ABS_TOP_BUILDDIR}/operations"

FrameCounter="${ABS_TOP_BUILDDIR}/examples/frame-counter"
GeglVideo="${ABS_TOP_BUILDDIR}/examples/gegl-video"

testcount=0

test() {
  ((testcount++))
  if "$@" 1> /dev/null; then
    echo "ok ${testcount}"
  else
    echo "not ok ${testcount} - $@"
  fi
}
endtests() {
  echo "1..${testcount}"
}

# Videos

test "${FrameCounter}" --video-codec mpeg4 --video-bit-rate 128            mpeg4-128kb.avi
test "${FrameCounter}" --video-codec mpeg4 --video-bit-rate 512            mpeg4-512kb.avi
test "${FrameCounter}" --video-codec mpeg4 --video-bit-rate 512            512kb.mp4
test "${FrameCounter}" --video-codec mpeg4 --video-bit-rate 128            128kb.mp4
test "${FrameCounter}" --video-codec mpeg4 --video-bit-rate 128 --fps 12   128kb-12fps.mp4
test "${FrameCounter}" --video-codec mpeg4 --video-bit-rate 128 --fps 100  128kb-100fps.mp4
test "${FrameCounter}"                     --video-bit-rate 512 --fps 28   512kb-28fps.ogv

# Images

for a in *.avi *.mp4 *.ogv ; do
  test "${GeglVideo}" $a -s 74 -e 74 -of $a-
done

endtests
