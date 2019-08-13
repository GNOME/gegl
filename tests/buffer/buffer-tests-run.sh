#!/bin/bash -e

mkdir -p output

test_name=$1
test_exe=$2
${test_exe}

LC_ALL=C diff \
    "${REFERENCE_DIR}/${test_name}.buf" \
              "output/${test_name}.buf"
