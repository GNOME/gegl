#!/bin/sh

set -e

outdir_normal="${ABS_BUILDDIR}/output"
outdir_opencl="${ABS_BUILDDIR}/output_cl"

mkdir -p "${outdir_normal}"
mkdir -p "${outdir_opencl}"

GEGL_USE_OPENCL=no  "${GEGL_BIN}" "${XML_FILE}" -o "${outdir_normal}/${OUT_FILE}"
GEGL_USE_OPENCL=yes "${GEGL_BIN}" "${XML_FILE}" -o "${outdir_opencl}/${OUT_FILE}"

"${GEGL_IMGCMP_BIN}" \
    "${outdir_normal}/${OUT_FILE}" \
    "${outdir_opencl}/${OUT_FILE}"
