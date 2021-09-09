#!/usr/bin/env python3
# 
# Copyright John Marshall 2020
#
# Flatten function is from a comment by Jordan Callicoat on 
# http://code.activestate.com/recipes/363051-flatten/ 
# 

from __future__ import print_function

import os
import sys
import argparse
import errno
import subprocess


class Args():
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '--verbose',
            action='store_true'
        )
        parser.add_argument(
            '--test-name',
            required=True,
            metavar='TEST_NAME',
            help='test name'
        )
        parser.add_argument(
            '--build-root',
            metavar='BUILD_ROOT',
            help='root directory for build'
        )
        parser.add_argument(
            '--output-dir',
            default='output',
            metavar='OUTPUT_DIR',
            help='directory for output files'
        )
        parser.add_argument(
            '--reference-path',
            metavar='REF_PATH',
            help='reference file or directory'
        )
        parser.add_argument(
            '--endian',
            metavar='big|little',
            help='endianness of reference files to use',
            default='unknown',
        )
        parser.add_argument(
            '--input-file',
            required=True,
            metavar='INPUT_FILE',
            help='input file for processing'
        )
        parser.add_argument(
            '--gegl-exe',
            metavar='GEGL',
            help='gegl program'
        )
        parser.add_argument(
            '--gegl-scale',
            metavar='GEGL_SCALE',
            help='gegl --scale value'
        )
        parser.add_argument(
            '--gegl-ops',
            nargs='*',
            metavar='OPS',
            help='gegl operations'
        )
        parser.add_argument(
            '--imgcmp-exe',
            metavar='IMGCMP',
            help='imgcmp program'
        )
        parser.add_argument(
            '--imgcmp-args',
            nargs='*',
            metavar='ARGS',
            help='imgcmp runtime arguments'
        )
        parser.add_argument(
            '--with-opencl',
            action='store_true',
            help='enable OpenCL' 
        )
        parser.add_argument(
            '--detect-opencl-exe',
            metavar='DETECT_OPENCL',
            help='OpenCL enabled check program'
        )
        parser.add_argument(
            '--generate-reference',
            action='store_true',
            help='generate non OpenCL reference for OpenCL test' 
        )

        # verbose
        self.verbose = parser.parse_args().verbose

        # test name
        self.test_name = parser.parse_args().test_name

        # set source dir from this file
        self.source_dir = os.path.realpath(
            os.path.join(os.path.dirname(__file__))
        )

        # get build directory from parameter
        if parser.parse_args().build_root:
            self.build_root = os.path.realpath(
                parser.parse_args().build_root
            )
        else:
            self.build_root = os.environ.get('ABS_TOP_BUILDDIR')
        if self.verbose: print('build root: %s' % self.build_root)

        # get output directory from parameter
        self.output_dir = os.path.realpath(
            os.path.join(parser.parse_args().output_dir)
        )
        if self.verbose: print('output dir: %s' % self.output_dir)

        # get reference directory from parameter
        if parser.parse_args().reference_path:
            self.reference_path = os.path.realpath(
                parser.parse_args().reference_path
            )
        else:
            self.reference_path = os.path.realpath(
                os.path.join(self.source_dir, 'reference')
            )
        if self.verbose: print('ref path: %s' % self.reference_path)
        self.endian = parser.parse_args().endian

        # input file from parameter
        if parser.parse_args().input_file:
            self.input_file = os.path.realpath(
                parser.parse_args().input_file
            )
        else:
            self.input_file = None
        if self.verbose: print('input files: %s' % self.input_file)

        # gegl args
        if parser.parse_args().gegl_scale:
            self.gegl_args = ['-s', parser.parse_args().gegl_scale]
        else:
            self.gegl_args = None

        # gegl operations
        self.gegl_ops = parser.parse_args().gegl_ops

        # gegl-imgcmp arguments 
        self.imgcmp_args = parser.parse_args().imgcmp_args

        # with OpenCL
        self.with_opencl = parser.parse_args().with_opencl

        # generate refrerence
        self.generate_ref = parser.parse_args().generate_reference
        if self.generate_ref and not self.with_opencl:
            self.generate_ref = False
            print('--generate-reference only valid with --with-opencl '
                + '- option ignored'
            )

        # executables
        if sys.platform == 'win32':
            exe_ext = '.exe'
        else:
            exe_ext = ''

        # gegl
        if parser.parse_args().gegl_exe:
            self.gegl_exe = parser.parse_args().gegl_exe
        else:
            self.gegl_exe = os.path.join(
                self.build_root, 'bin', 'gegl' + exe_ext
            )
        self.gegl_exe = os.path.realpath(self.gegl_exe)
        if self.verbose: print('gegl exe: %s' % self.gegl_exe)

        # imgcmp
        if parser.parse_args().imgcmp_exe:
            self.imgcmp_exe = parser.parse_args().imgcmp_exe
        else:
            self.imgcmp_exe = os.path.join(
                    self.build_root, 'tools', 'gegl-imgcmp' + exe_ext
                )
        self.imgcmp_exe = os.path.realpath(self.imgcmp_exe)
        if self.verbose: print('imgcmp exe: %s' % self.imgcmp_exe)

        # detect opencl
        if parser.parse_args().detect_opencl_exe:
            self.detect_ocl_exe = parser.parse_args().detect_opencl_exe
        else:
            self.detect_ocl_exe = os.path.realpath(os.path.join(
                    self.build_root, 'tools', 'detect_opencl' + exe_ext
                )
            )
        self.detect_ocl_exe = os.path.realpath(self.detect_ocl_exe)
        if self.verbose: print(
            'detect OpenCL exe: %s' % self.detect_ocl_exe
        )


def flatten(l, ltypes=(list, tuple)):
    ltype = type(l)
    l = list(l)
    i = 0
    while i < len(l):
        while isinstance(l[i], ltypes):
            if not l[i]:
                l.pop(i)
                i -= 1
                break
            else:
                l[i:i + 1] = l[i]
        i += 1
    return ltype(l)

def main():
    args = Args()

    # set test environment
    test_env = os.environ.copy()
    if args.with_opencl:
        try:
            subprocess.check_output(args.detect_ocl_exe, env=test_env)
        except:
            print('Skipping - OpenCL not available')
            sys.exit(77)
        if args.verbose: print('Running with OpenCL')

    if not os.path.exists(args.input_file):
        print('Skipping - cannot find input file: %s' % args.input_file)
        sys.exit(77)

    if not os.path.exists(args.gegl_exe):
        print('Skipping - cannot find gegl: %s' % args.gegl_exe)
        sys.exit(77)

    if not os.path.exists(args.imgcmp_exe):
        print('Skipping - cannot find imgcmp: %s' % args.imgcmp_exe)
        sys.exit(77)

    # find reference file for comparison
    if args.generate_ref:
        file_ext = '.png'
        if not os.path.exists(args.reference_path):
            try:
                os.makedirs(args.reference_path, 0o700)
            except OSError as err:
                if err.errno != errno.EEXIST:
                    raise
        if args.reference_path == args.output_dir:
            ref_file = os.path.join(
                args.reference_path, args.test_name + '_ref' + file_ext
            )
        else:
            ref_file = os.path.join(
                args.reference_path, args.test_name + file_ext
            )
    else:
        if os.path.isfile(args.reference_path):
            ref_file = args.reference_path
            file_ext = os.path.splitext(ref_file)[1]
        elif os.path.isdir(args.reference_path):
            # find reference file matching test name in ref dir
            for file_ext in [
                '.png',
                '.hdr',
                '.%s-endian.gegl' % args.endian,
            ]:
                ref_file = os.path.join(
                    args.reference_path, args.test_name + file_ext
                )
                if os.path.exists(ref_file):
                    break
            else:
                print('Skipping - cannot find test reference file')
                sys.exit(77)
        else:
            print('Skipping - cannot find test reference file')
            sys.exit(77)
    if args.verbose: print('reference file: %s' % ref_file)

    output_file = os.path.join(
        args.output_dir, args.test_name + file_ext
    )
    if args.verbose: print('output file: %s' % output_file)
    if not os.path.exists(args.output_dir):
        try:
            os.makedirs(args.output_dir, 0o700)
        except OSError as err:
            if err.errno != errno.EEXIST:
                raise

    if args.generate_ref:
        ref_cmd = [args.gegl_exe, args.input_file, '-o', ref_file]
        if args.gegl_args: ref_cmd += args.gegl_args
        if args.gegl_ops: ref_cmd += ['--', args.gegl_ops]
        ref_cmd = flatten(ref_cmd)

    gegl_cmd = [args.gegl_exe, args.input_file, '-o', output_file]
    if args.gegl_args: gegl_cmd += args.gegl_args
    if args.gegl_ops: gegl_cmd += ['--', args.gegl_ops]
    gegl_cmd = flatten(gegl_cmd)

    imgcmp_cmd = [args.imgcmp_exe, ref_file, output_file]
    if args.imgcmp_args: imgcmp_cmd += args.imgcmp_args
    imgcmp_cmd = flatten(imgcmp_cmd)

    no_ocl_env = test_env
    no_ocl_env["GEGL_USE_OPENCL"] = "no"

    if args.verbose:
        if args.generate_ref: 
            print('ref cmd: %s' % ' '.join(ref_cmd))
        print('gegl cmd: %s' % ' '.join(gegl_cmd))
        print('imgcmp cmd: %s' % ' '.join(imgcmp_cmd))
    sys.stdout.flush()

    try:
        # run gegl to produce reference image
        if args.generate_ref:
            subprocess.check_call(ref_cmd, env=no_ocl_env)
        # run gegl to produce test image
        subprocess.check_call(gegl_cmd, env=test_env)
        # run imgcmp to compare output against reference
        subprocess.check_call(imgcmp_cmd, env=no_ocl_env)
    except KeyboardInterrupt:
        raise
    except subprocess.CalledProcessError as err:
        sys.stdout.flush()
        sys.exit(err.returncode)
    
    sys.stdout.flush()
    sys.exit(0)


if __name__ == '__main__':
  main()
