#!/bin/bash

set -e
set -x

meson _build
ninja -C _build
