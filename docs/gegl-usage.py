#!/usr/bin/env python3
#
# Copyright John Marshall 2021
#
import os
import sys
import argparse
import subprocess

class Args():
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '--gegl-exe',
            metavar='GEGL',
            help='gegl program'
        )
        parser.add_argument(
            '--output',
            metavar='OUTPUT_FILE',
            help='output file - otherwise output to STDOUT'
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
        # output file
        if parser.parse_args().output:
            self.output = os.path.realpath(parser.parse_args().output)
        else:
            self.output = None


def main():
    args = Args()

    if args.output:
        try:
            out_file = open(os.path.realpath(args.output), 'w')
        except IOError:
            print('cannot open output file %s' % args.output,
                  sys.stderr)
            sys.exit(1)
    else:
        out_file = sys.stdout

    subprocess.call(
        [args.gegl_exe, '--help'],
        stderr = subprocess.STDOUT,
        stdout = out_file
    )

    sys.exit(0)


if __name__ == '__main__':
    main()
