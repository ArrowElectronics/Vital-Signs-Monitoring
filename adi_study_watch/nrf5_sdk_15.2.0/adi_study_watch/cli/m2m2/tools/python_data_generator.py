#!/usr/bin/env python3

import sys, os
if sys.version_info[0] < 3:
    raise "Must be using Python 3"

sys.path.insert(0, "../inc/master_definitions")
import data_definition_generator

definitions_dir = os.path.abspath('../inc/master_definitions')
output_dir = os.path.abspath('../inc')
generator_name = os.path.basename(data_definition_generator.__file__)

definitions_files = []

# Use os.listdir() so that we don't recurse into subdirectories
# for f in os.listdir(os.path.join(os.getcwd(), definitions_dir)):
for f in os.listdir(definitions_dir):
    if f.endswith(".py") and generator_name not in f:
        definitions_files.append(os.path.join(definitions_dir, f))

parser = data_definition_generator.data_definition_generator(output_dir=output_dir)
parser.read_files(definitions_files)
parser.parse(["c++", "c", "python"])
