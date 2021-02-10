#!/usr/bin/env python3

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses/>.
#
#
# Utility script to re-implement gobj2dot.rb in python with extra
# option to send the output to a new file.
#
# Copyright (C) 2009 Henrik Akesson
# Copyright (C) 2021 John Marshall

from __future__ import print_function

import os
import sys
import argparse
import glob
import re

header = '''digraph G {
        rankdir = RL

        fontname = "Bitstream Vera Sans"
        fontsize = 8

        node [ shape = "record" ]

        edge [ arrowhead = empty ]'''

footer = '}'

re_def_type = re.compile(
    r'G_DEFINE_TYPE\s*\(\s*\w+,\s*(\w+),\s*(\w+)\s*\)',
    re.DOTALL
)
re_def_type_code = re.compile(
    r'G_DEFINE_TYPE_WITH_CODE\s*\(\s*(\w+).*G_IMPLEMENT_INTERFACE\s*\s*\(\s*(\w+)',
    re.DOTALL
)
re_interface = re.compile(
    r'G_TYPE_INTERFACE\s*,\s*\"([^\"]+)\"',
    re.DOTALL
)


class Args():
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '--output',
            metavar='OUTPUT_FILE',
            help='output file - otherwise output to STDOUT'
        )
        parser.add_argument(
            'PATH',
            help='search path'
        )

        self.path = os.path.realpath(parser.parse_args().PATH)
        if parser.parse_args().output:
            self.output = os.path.realpath(parser.parse_args().output)
        else:
            self.output = None

def snake_to_camel(name):
    return re.sub(r'(?:^|_)([a-z])', lambda x: x.group(1).upper(), name)

def gegl_type_to_camel(name):
    return snake_to_camel('_'.join(name.split('_TYPE_')).lower())


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

    print(header, file = out_file)

    for filename in glob.iglob(os.path.join(args.path, '**', '*.[ch]'),
                               recursive = True):
        f = open(filename, mode='r', encoding='utf-8')
        content = f.read()
        f.close()
        match = re.search(re_def_type, content)
        if match:
            if match.groups()[1] != 'G_TYPE_OBJECT':
                print(
                    '\t' + snake_to_camel(match.groups()[0]) +
                    ' -> ' + gegl_type_to_camel(match.groups()[1]),
                    file = out_file
                )
        else:
            match = re.search(re_def_type_code, content)
            if match:
                print(
                    '\t' + match.groups()[0] +
                    ' -> ' + gegl_type_to_camel(match.groups()[1]) +
                    ' [ style = dashed ]',
                    file = out_file
                )
            else:
                match = re.search(re_interface, content)
                if match:
                    print(
                    '\t' + match.groups()[0] +
                    ' [ label = \"{\\<\\<interface\\>\\>\\l' +
                    match.groups()[0] + '}\" ]',
                    file = out_file
                )


    print(footer, file = out_file)

    sys.exit(0)


if __name__ == "__main__":
  main()