#!/usr/bin/env python3


import os, importlib.util, inspect, ctypes, sys
from types import ModuleType

from pprint import pprint
file_write = True

import ast

class get_variables(ast.NodeVisitor):
    def __init__(self):
        self.definitions = []
    def visit_Name(self, node):
        if isinstance(node.ctx, ast.Store):
            self.definitions.append((node.lineno, node.id))

    def get_line_nums(self, module):
        self.visit(module)
        return sorted(self.definitions, key=lambda x: x[0])


class data_obj():
    def __init__(self, name):
        self.name = name
        self.structs = []
        self.enums = []
        self.dependencies = []

class data_definition_generator():
    supported_languages = [ "c",
                            "c++",
                            "python"]
    tmp_module_name = "THIS_IS_A_VERY_VERBOSE_USER_MODULE_NAME_TO_AVOID_CONFLICTS"
    int_type_map = {
                    ctypes.c_uint8:     "uint8_t",
                    ctypes.c_uint16:    "uint16_t",
                    ctypes.c_uint32:    "uint32_t",
                    ctypes.c_int8:      "int8_t",
                    ctypes.c_int16:     "int16_t",
                    ctypes.c_int32:     "int32_t"
                    }

    def __init__(self, output_dir = None):
        if not isinstance(output_dir, str):
            raise valueError("'output_dir' must be a string!")
        self.base_output_dir = os.path.abspath(output_dir)

    def read_files(self, input_filenames = None,):
        if not isinstance(input_filenames, list):
            raise ValueError("'input_filenames' must be a list containing at least one filename!")

        self.input_defs = []
        input_filenames = [os.path.abspath(x) for x in input_filenames]
        for filename in input_filenames:
            sys.path.append(os.path.dirname(filename))
            m = self._open_module(filename)
            d = self._extract_data_objs(m)
            for name, obj in inspect.getmembers(m):
                if isinstance(obj, ModuleType) and name != m.__name__:
                    # Check and see if we're already going to parse this dependency. If not, add it to the list of files to parse
                    dep_full_name = os.path.join(os.path.dirname(filename), name + ".py")
                    if dep_full_name not in input_filenames:
                        print("Found a dependency '{}' that isn't explicitly being converted! Adding it to the conversion list...".format(name))
                        input_filenames.append(dep_full_name)
                    # Find all the data items inside the dependency
                    dep_m = self._open_module(dep_full_name)
                    dep_d = self._extract_data_objs(dep_m)
                    d.dependencies.append(dep_d)
            self.input_defs.append(d)

    def _open_module(self, module_fname):
        spec = importlib.util.spec_from_file_location(self.tmp_module_name, module_fname)
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        return m

    def _extract_data_objs(self, module):
        filename = module.__file__
        d = data_obj(os.path.splitext(filename)[0])

        # Use the AST module to find all of the variables defined in the module and their line numbers
        with open(module.__file__, "r") as f:
            m = ast.parse(f.read())

        # returns a list of tuples of all variable names in m sorted by their line number
        defs = get_variables().get_line_nums(m)

        for line_num, item in defs:
            # Instantiate the identifier
            if hasattr(module, item):
                i = getattr(module, item)
                # Make sure it's a non-Python-internal dictionary
                if isinstance(i, dict) and not item.startswith("__") and not item.endswith("__"):
                    # Store the variable's name in the dictionary so it's easy to access later
                    i["name"] = item
                    if "struct_fields" in i:
                        for field in i["struct_fields"]:
                            if "name" in field and field["name"] == None:
                                if "length" in field:
                                    print("WARNING::{}::{} Detected a field in struct '{}' with 'name' == None and a 'length'. The length will be ignored.".format(os.path.split(filename)[-1], line_num, i["name"]))
                                    del field["length"]
                        d.structs.append(i)
                    elif "enum_values" in i:
                        implicit_enum_value = 0
                        for index, pair in enumerate(i["enum_values"]):
                            # If an enum entry is declared with just a name, turn it into a tuple for processing
                            if isinstance(pair, str):
                                # We can't modify pair directly, so replace it, and then re-set it
                                i["enum_values"][index] = (pair,)
                                pair = i["enum_values"][index]

                            # If an enum entry is declared as a 1-tuple with only a name
                            if isinstance(pair, tuple) and len(pair) == 1:
                                print("INFO::{}::{} Detected an enum member '{}::{}' without an explicit value. Extrapolating from previous values...".format(os.path.split(filename)[-1], line_num, i["name"], pair[0]))
                                i["enum_values"][index] = (pair[0], implicit_enum_value)
                                implicit_enum_value += 1
                            # A fully-specified enum entry
                            elif len(pair) == 2:
                                implicit_enum_value = pair[1] + 1
                            else:
                                print("FATAL ERROR::{}::{} Incorrect enum value declaration: '{}::{}'".format(os.path.split(filename)[-1], line_num, i["name"], pair[0]))
                                sys.exit(-1)
                        d.enums.append(i)
                    else:
                        raise ValueError("Found an object that doesn't look like either a struct or an enum!\n"
                                        "object name:{}".format(item))
        if len(d.structs) == 0 and len(d.enums) == 0:
            raise ValueError("Looks like no structs or enums were found to parse!")
        return d

    def _w(self, msg):
        if file_write:
            sys.__stdout__.write(msg)

    def _resolve_type_name(self, d_obj, type_obj, resolve_ctypes=True):
        name = self._resolve_enum_name(d_obj, type_obj, resolve_ctypes)
        if name != None:
            return name
        # Handle the case where a type is defined in another module
        if isinstance(type_obj, dict) and "type" in type_obj:
            return self._resolve_type_name(d_obj, type_obj["type"], resolve_ctypes)

        if not resolve_ctypes and not isinstance(type_obj, dict):
            return type_obj.__name__
        else:
            # Take a "type" and turn it into a C compatible string representation
            for t, s in self.int_type_map.items():
                if isinstance(type_obj, dict):
                    if not "name" in type_obj:
                    # if type_obj["name"] == "":
                        return type_obj["type"]
                    else:
                        return type_obj["name"]
                if isinstance(type_obj(), t):
                    return s
        return None

    def _parse_struct(self, d_obj, struct, resolve_ctypes=True):
        '''
        Takes a struct dictionary that might have other structs as fields, and
        returns a list of fields in that struct, with all nested fields of struct
        type resolved to their basic types.
        if resolve_ctypes is set, ctypes types will be converted to C/C++ stdint strings
        i.e.:
        ###########################################################
        my_struct_1 = {
            "struct_fields": [ {
                          "name": "base_field",
                          "type": c_uint8,
                        }
                        ]
        }
        my_struct_t = {
          "struct_fields": [{
                                "type": my_struct_1,
                            },
                            ]
        }
        ###########################################################
        becomes
        ###########################################################
        my_struct_1 = {
            "struct_fields": [ {
                          "name": "base_field",
                          "type": c_uint8,
                        }
                        ]
        }
        my_struct_t = {
          "struct_fields": [{
                                "name": "base_field",
                                "type": c_uint8,
                            }
                            ]
        }

        '''
        struct_fields = []
        for field in struct["struct_fields"]:
            resolved_field = {}
            if "length" in field and field["length"] != None:
                # This field is an array, so we need its length
                resolved_field["length"] = field["length"]
            if "type" in field and self._is_struct(field["type"]):
                # This field has a user-defined struct type
                if "length" in field and field["length"] != None:
                    # A regular array of structs; just plug it in
                    resolved_field["name"] = field["name"]
                    resolved_field["type"] = self._resolve_type_name(d_obj, field["type"], resolve_ctypes)
                if not "name" in field:
                    # The field needs a name entry.
                    raise Exception("Error! A field in struct '{}' does not have a name!".format(struct["name"]))
                elif field["name"] == None:
                    # This is an anonymous struct; resolve its members (plug the definition of its type into the parent struct)
                    struct_fields.extend(self._parse_struct(d_obj, field["type"], resolve_ctypes))
                elif not "length" in field:
                    # Avoid double-counting regular array structs
                    struct_fields.extend([{"name":field["name"],
                                            "type":field["type"]["name"]}])
            else:
                # The type of this field isn't a struct
                resolved_field["name"] = field["name"]
                resolved_field["type"] = self._resolve_type_name(d_obj, field["type"], resolve_ctypes)

            if len(resolved_field) != 0:
                struct_fields.append(resolved_field)
        return struct_fields


    def _resolve_enum_name(self, d_obj, enum_obj, resolve_ctypes = True):
        if not resolve_ctypes:
            for t in self.int_type_map:
                if isinstance(enum_obj, dict) and "type" in enum_obj:
                    return enum_obj["type"].__name__
                if isinstance(enum_obj, t):
                    return enum_obj.__name__
        if enum_obj in d_obj.enums:
            return enum_obj["name"]
        # See if there's a type defined in a dependency
        for dep in d_obj.dependencies:
            for enum in dep.enums:
                # Slightly hacky check to see if this enum object is defined in a dependency
                # Ideally this would be done using names (although checking that all the enums are identical is probably better, actually...)
                if isinstance(enum_obj, dict) and "enum_values" in enum_obj and enum["enum_values"] == enum_obj["enum_values"]:
                    return enum["name"]
        return None


    def _sizeof(self, obj):
        if obj in self.int_type_map:
            return ctypes.sizeof(obj)

    def _parse_cpp(self):
        self._parse_c_cpp(True)

    def _is_enum(self, obj):
        return (isinstance(obj, dict) and "enum_values" in obj)

    def _is_struct(self, obj):
        return (isinstance(obj, dict) and "struct_fields" in obj)

    def _parse_c_cpp(self, cpp=False):
        for d in self.input_defs:
            out_fname =  d.name
            if cpp:
                ext = ".hpp"
            else:
                ext = ".h"
            out_fname += ext
            if cpp:
                subdir = "cpp"
            else:
                subdir = "c"
            outdir = os.path.join(self.base_output_dir, subdir)
            if not os.path.exists(outdir):
                os.makedirs(outdir)
            # Put out the output file in the base_output_dir with the same name as the original file
            out_fname = os.path.join(outdir, os.path.split(out_fname)[-1])
            with open(out_fname, "w+") as f_o:
                # File headings
                f_o.write(
                "// #############################################################################\n"
                "// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!\n"
                "// #############################################################################\n"
                "#pragma once\n\n")
                for dep in d.dependencies:
                    f_o.write("#include \"{}\"\n".format(os.path.split(dep.name)[-1] + ext))
                f_o.write("#include <stdint.h>\n\n")
                f_o.write(
                "\n/* Explicitly enforce struct packing so that the nested structs and unions are laid out\n"
                "    as expected. */\n"
                "#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__\n"
                "// DELIBERATELY BLANK\n"
                "#else\n"
                "#error \"WARNING! Your compiler might not support '#pragma pack(1)'! \\\n"
                "  You must add an equivalent compiler directive to the file generator!\"\n"
                "#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__\n"
                "#pragma pack(1)\n"
                "\n"
                )
                if not cpp:
                    # This hacky static assert has to be added for C to make sure enums are the size we expect. C++ has a built in static assertion function
                    f_o.write("#ifndef STATIC_ASSERT_PROJ\n")
                    f_o.write("#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]\n")
                    f_o.write("#endif // STATIC_ASSERT_PROJ\n")
                for enum in d.enums:
                    implicit_enum_value = 0
                    if cpp:
                        # Enums are strongly typed in C++
                        f_o.write("enum {}:{} {{\n".format(enum["name"], self.int_type_map[enum["type"]]))
                    else:
                        # Typedef so we get some type checking in C
                        f_o.write("typedef enum {} {{\n".format(enum["name"]))

                    for pair in enum["enum_values"]:
                        f_o.write("  {} = {},\n".format(pair[0], pair[1]))

                    if cpp:
                        f_o.write("};\n")
                    else:
                        f_o.write("}} {};\n".format(enum["name"]))

                    if cpp:
                        f_o.write('static_assert(sizeof({0}) == {1}, "Enum \'{0}\' has an incorrect size!");\n'.format(enum["name"], self._sizeof(enum["type"])))
                    else:
                        f_o.write("STATIC_ASSERT_PROJ(sizeof({0}) == {1}, INCORRECT_SIZE_{0});\n".format(enum["name"], self._sizeof(enum["type"])))
                    f_o.write("\n")

                for struct in d.structs:
                    # print("STRUCT:")
                    # pprint(struct)
                    struct_text_lines = []
                    struct_indentation = 2
                    if "big_endian" in struct and struct["big_endian"] == True:
                        f_o.write("// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n")
                        f_o.write("// @@  NOTE: THE FIELDS IN THIS STRUCTURE ARE BIG ENDIAN!  @@\n")
                        f_o.write("// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n")

                    if cpp:
                        f_o.write("struct {} {{\n".format(struct["name"]))
                    else:
                        f_o.write("typedef struct _{} {{\n".format(struct["name"]))

                    # print("FIELDS IN STRUCT:")
                    for field in self._parse_struct(d, struct):
                        arr_str = ""
                        arr_comment = ""
                        # pprint(field)
                        if "length" in field and field["length"] != None:
                            if cpp and (field["length"] == 0):
                                arr_comment = "// NOTE: THIS FIELD IS INTENDED TO BE OF VARIABLE LENGTH! \n        // NOTE: Use offsetof({0}, {1}) instead of sizeof({0})".format(struct["name"], field["name"])
                                arr_str = "[1]"
                            else:
                                arr_str = "[{}]".format(field["length"])
                        struct_text_lines.append((field["type"], field["name"], arr_str, arr_comment))
                    for line in struct_text_lines:
                        # Broken out so it's easier to add nice indentation that makes all the entries line up
                        f_o.write("  {}{}{}{}; {}\n".format(line[0], struct_indentation * " ", line[1], line[2], line[3]))
                    if cpp:
                        f_o.write("};\n")
                    if not cpp:
                        f_o.write("}} {};\n".format(struct["name"]))
                    f_o.write("\n")

                # File footings
                f_o.write(
                        "// Reset struct packing outside of this file\n"
                        "#pragma pack()\n"
                        )

    def _parse_python(self):
        # File headings
        for d in self.input_defs:
            out_fname =  d.name + "_def"
            ext = ".py"
            outdir = os.path.join(self.base_output_dir, "python")
            if not os.path.exists(outdir):
                os.makedirs(outdir)
            # Put out the output file in the base_output_dir with the same name as the original file
            out_fname = os.path.join(outdir, os.path.split(out_fname)[-1])
            with open(os.path.join(self.base_output_dir, out_fname + ext), "w+") as f_o:
                f_o.write("from ctypes import *\n\n")
                for dep in d.dependencies:
                    f_o.write("from {}_def import *\n\n".format(os.path.split(dep.name)[-1]))
                for enum in d.enums:
                    f_o.write("class {}({}):\n".format(enum["name"], enum["type"]().__class__.__name__))
                    for pair in enum["enum_values"]:
                        f_o.write("    {} = 0x{:X}\n".format(pair[0], pair[1]))
                    f_o.write("\n")

                for struct in d.structs:
                    parsed_structs = self._parse_struct(d, struct, False)
                    variable_len_struct = False
                    # Look ahead to see if we need to define this as a class factory instead of a plain class
                    if "big_endian" in struct and struct["big_endian"] == True:
                        py_struct_base_class = "BigEndianStructure"
                    else:
                        py_struct_base_class = "Structure"
                    for field in parsed_structs:
                        if "length" in field and field["length"] == 0:
                            variable_len_struct = True
                            break
                    if variable_len_struct:
                        f_o.write("def {}(array_size):\n".format(struct["name"]))
                        f_o.write("  class {}_internal({}):\n".format(struct["name"], py_struct_base_class))
                    else:
                        f_o.write("class {}({}):\n".format(struct["name"], py_struct_base_class))
                    f_o.write("    _pack_ = 1\n")
                    f_o.write("    _fields_ = [\n")
                    struct_text_lines = []
                    for field in self._parse_struct(d, struct, False):
                        arr_str = ""
                        if "length" in field:
                            if variable_len_struct:
                                arr_str = " * array_size"
                            else:
                                arr_str = " * {}".format(field["length"])
                        struct_text_lines.append((field["name"], field["type"], arr_str))

                    for line in struct_text_lines:
                        f_o.write("              (\"{}\", {}),\n".format(line[0], line[1] + line[2]))
                    f_o.write("              ]\n")
                    if variable_len_struct:
                        f_o.write("  return {}_internal()\n".format(struct["name"]))
                    f_o.write("\n")

    def parse(self, languages = None):
        if not languages:
            raise ValueError("You did not specify at least one language to parse!")
        for l in languages:
            if not isinstance(l, str) or l not in self.supported_languages:
                raise ValueError("You provided an unknown language '{}'".format(l))

        if "c++" in languages:
            self._parse_cpp()
        if "c" in languages:
            self._parse_c_cpp()
        if "python" in languages:
            self._parse_python()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(
        "Automatically generate cross-compatible C, C++, and Python definition files from a simple Python template file.")
    parser.add_argument('-if', '--input_files',
        required=True,
        nargs='+',
        help='A list of files to be converted. Should be specified using a relative path.')
    parser.add_argument('-od', '--output_dir',
        required=True,
        help='The output directory where the generated files will be placed.\n'
        'Should be specified using a relative path.\n'
        'Each output file type will be placed in a subdirectory within the output directory.\n'
        'If the subdirectories do not exist, they will be created\n')
    parser.add_argument('-langs', '--target_languages',
        required=True,
        nargs='+',
        choices=data_definition_generator.supported_languages,
        help='The target language(s) to parse.')
    args = parser.parse_args()


    g = data_definition_generator(args.output_dir)
    g.read_files(args.input_files)
    g.parse(args.target_languages)
