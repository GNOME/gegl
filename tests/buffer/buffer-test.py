#!/usr/bin/env python3

from __future__ import print_function

import os
import sys
import argparse
import errno
import filecmp
import subprocess

class Args():
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '--reference-dir',
            metavar='REF_DIR',
            help='directory holding reference images'
        )
        parser.add_argument(
            'TEST',
            metavar='TEST_NAME',
            help='buffer test file to run'
        )

        self.test_exe = os.path.realpath(parser.parse_args().TEST)
        self.test_name = os.path.splitext(
            os.path.basename(self.test_exe)
        )[0]
        self.test_file = self.test_name + '.buf'

        if parser.parse_args().reference_dir:
            self.reference_dir = os.path.realpath(
                parser.parse_args().reference_dir
            )
        else:
            self.reference_dir = os.path.realpath('reference')
        print(self.reference_dir)


def main():
    args = Args()

    if not os.path.exists(args.test_exe):
        print('Skipping - cannot find test exe: %s' % args.test_exe)
        sys.exit(77)

    ref_file = os.path.join(args.reference_dir, args.test_file)
    if not os.path.exists(ref_file):
        print('Skipping - cannot find buffer reference file')
        sys.exit(77)

    output_dir = os.path.realpath('output') 
    if not os.path.exists(output_dir):
        try:
            os.makedirs(output_dir, 0o700)
        except OSError as err:
            if err.errno != errno.EEXIST:
                raise

    try:
        subprocess.call(args.test_exe)
    except subprocess.CalledProcessError as err:
        print(err.returncode)

    if filecmp.cmp(
        ref_file,
        os.path.join(output_dir, args.test_file),
        shallow=False
    ):
        sys.exit(0)
    else:
        print(': buffer output does not match reference')
        sys.exit(1)
        

if __name__ == "__main__":
  main()