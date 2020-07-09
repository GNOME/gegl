#!/usr/bin/env python3
#
# Utility script to run an executable in a given working directory
# 
# Copyright John Marshall 2020
#
import os
import sys
import argparse
import subprocess

class Args():
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '--output-dir',
            metavar = 'DIR',
            help = 'output directory'
        )
        parser.add_argument(
            '--redirect',
            action='store_true',
            help = 'redirect stderr to stdout' 
        )
        parser.add_argument(
            'EXE_FILE',
            metavar = 'EXE',
            help = 'executable to run'
        )

        if parser.parse_args().output_dir:
            self.output_dir = os.path.realpath(
                parser.parse_args().output_dir
            )
        else:
            self.output_dir = os.getcwd()
        self.redirect = parser.parse_args().redirect
        self.run_exe = os.path.realpath(parser.parse_args().EXE_FILE)

class ChgDir():
    '''Context manager for changing the current working directory'''
    def __init__(self, new_path):
        self.new_path = os.path.realpath(new_path)

    def __enter__(self):
        self.saved_path = os.getcwd()
        os.chdir(self.new_path)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.saved_path)


def main():
    args = Args()

    try:
        with ChgDir(args.output_dir):
            subprocess.check_call(args.run_exe,
                                  stderr=subprocess.STDOUT)
    except KeyboardInterrupt:
        raise
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)

    sys.exit(0)


if __name__ == '__main__':
    main()
