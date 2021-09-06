#!/usr/bin/env python3


import os, importlib.util, inspect, ctypes, sys
from operator import itemgetter
from numbers import Integral

file_write = True

class data_obj():
    def __init__(self, name):
        self.name = name
        self.structs = []
        self.enums = []
        self.defines = []
        self.dependencies = []

class data_definition_generator():
    supported_languages = [ "c",
                            "c++",
                            "python"]
    data_type_map = {
                    ctypes.c_uint8:     "uint8_t",
                    ctypes.c_uint16:    "uint16_t",
                    ctypes.c_uint32:    "uint32_t",
                    ctypes.c_int8:      "int8_t",
                    ctypes.c_int16:     "int16_t",
                    ctypes.c_int32:     "int32_t",
                    ctypes.c_float:     "float"
                    }

    define_exclusion_list = ["DEFAULT_MODE", "RTLD_GLOBAL", "RTLD_LOCAL"]

    def __init__(self, output_dir = None):
        if not isinstance(output_dir, str):
            raise ValueError("'output_dir' must be a string!")
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
            for name, obj in inspect.getmembers(m, inspect.ismodule):
                if name != m.__name__:
                    # Check and see if we're already going to parse this dependency. If not, add it to the list of files to parse
                    dep_full_name = os.path.join(os.path.dirname(filename), name + ".py")
                    if dep_full_name not in input_filenames:
                        print("Found a dependency '{}' that isn't explicitly being converted! Adding it to the conversion list...".format(name))
                        # Find all the data items inside the dependency
                    dep_m = self._open_module(dep_full_name)
                    dep_d = self._extract_data_objs(dep_m)
                    d.dependencies.append(dep_d)
            self.input_defs.append(d)

    def _open_module(self, module_fname):
        spec = importlib.util.spec_from_file_location(module_fname, module_fname)
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        importlib.invalidate_caches()
        return m

    def _extract_data_objs(self, module):
        d = data_obj(os.path.splitext(module.__file__)[0])
        structs = []
        enums = []
        for name, c in inspect.getmembers(module):
            if inspect.isclass(c):
                if c.__module__ == module.__file__:
                    tmp_name = os.path.splitext(os.path.split(module.__file__)[-1])[0] #module.__file__.split("/")[-1][:-3]
                    mod = __import__(tmp_name, fromlist=[c.__name__])
                    klass = getattr(mod, c.__name__)
                    lineno = inspect.getsourcelines(klass)[1]
                    attrs = {k: v for k, v in vars(c).items() if not k.startswith("__")}
                    #handle structs
                    if "fields" in attrs:
                        parent_cls = inspect.getmro(c)[1]
                        struct = {"name": name, "parent_cls": parent_cls, "values": attrs["fields"]}
                        structs.append((struct, lineno))
                    #handle enums
                    else:
                        parent_cls = inspect.getmro(c)[1]
                        enum = {"name": name, "parent_cls": parent_cls,
                            "values": list(sorted(attrs.items(), key=lambda item: item[1]))}
                        enums.append((enum, lineno))
            #handle defines
            else:
                if isinstance(c, Integral) and name not in self.define_exclusion_list:
                    define = {"name": name, "values": c}
                    d.defines.append(define)

        #order structs and enums by lineno
        d.structs = [s for s, l in sorted(structs, key=itemgetter(1))]
        d.enums = [e for e, l in sorted(enums, key=itemgetter(1))]
        
        if len(d.structs) == 0 and len(d.enums) == 0 and len(d.defines) == 0:
            raise ValueError("Looks like no structs or enums were found to parse!")
        
        return d

    def _resolve_enum_name(self, d_obj, enum_obj, resolve_ctypes=True):
        if not resolve_ctypes:
            new_enum = self._find_enum(d_obj, enum_obj)
            if new_enum is not None:
                for t, s in self.data_type_map.items():
                    if issubclass(new_enum['parent_cls'], t):
                        return t.__name__
            else:
                return enum_obj.__name__
        enum_obj_name = enum_obj.__name__
        enum_names = [enum["name"] for enum in d_obj.enums]
        if enum_obj_name in enum_names:
            return enum_obj_name
        # See if it's defined in a dependency
        for dep in d_obj.dependencies:
            enum_dep_names = [enum["name"] for enum in dep.enums]
            if enum_obj_name in enum_dep_names:
                return enum_obj_name
        return None

    def _resolve_type_name(self, d_obj, type_obj, resolve_ctypes=True):
        name = self._resolve_enum_name(d_obj, type_obj, resolve_ctypes)
        if name != None:
            return name
        type_obj_name = type_obj.__name__
        struct_names = [struct["name"] for struct in d_obj.structs]
        if type_obj_name in struct_names:
            return type_obj_name
        # Handle the case where a type is defined in another module
        for dep in d_obj.dependencies:
            struct_dep_names = [struct["name"] for struct in dep.structs]
            if type_obj_name in struct_dep_names:
                return type_obj_name
        # Take a "type" and turn it into a C compatible string representation
        for t, s in self.data_type_map.items():
            if isinstance(type_obj(), t):
                return s
        return None

    def _parse_struct(self, d_obj, struct, resolve_ctypes=True):
        struct_fields = []
        for field in struct["values"]:
            resolved_field = {}
            # This field is an array, so we need its length
            if issubclass(field[1], ctypes.Array):
                resolved_field["length"] = field[1]._length_
                #Store the class type in tmp_type
                tmp_type = field[1]._type_
            else:
                tmp_type = field[1]
            #This field has a user-defined struct type
            if not self._is_ctypes_primitive(tmp_type):
                # This is an anonymous struct; resolve its members (plug the definition of its type into the parent struct)
                if field[0] == None:
                    new_struct = self._find_struct(d_obj, tmp_type)
                    struct_fields.extend(self._parse_struct(d_obj, new_struct, resolve_ctypes))
                else:
                    if "length" in resolved_field:
                        resolved_field["name"] = field[0]
                        resolved_field["type"] = self._resolve_type_name(d_obj, tmp_type, resolve_ctypes)
                    else:
                        # Avoid double-counting regular array structs
                        struct_fields.extend([{"name":field[0],
                                                "type":self._resolve_type_name(d_obj, tmp_type, resolve_ctypes)}])
            else:
                # The type of this field isn't a struct
                resolved_field["name"] = field[0]
                resolved_field["type"] = self._resolve_type_name(d_obj, tmp_type, resolve_ctypes)

            if len(resolved_field) != 0:
                struct_fields.append(resolved_field)
        return struct_fields


    def _sizeof(self, obj):
        if obj in self.data_type_map:
            return ctypes.sizeof(obj)

    def _is_ctypes_primitive(self, obj):
        return issubclass(obj, tuple(self.data_type_map.keys()))

    def _find_struct(self, d_obj, type_obj):
        type_obj_name = type_obj.__name__
        struct_pairs = [(struct["name"], struct) for struct in d_obj.structs]
        for name, s in struct_pairs:
            if type_obj_name == name:
                return s
        #Handle the case where a struct is defined in another module
        for dep in d_obj.dependencies:
            struct_dep_pairs = [(struct["name"], struct) for struct in dep.structs]
            for name, s in struct_dep_pairs:
                if type_obj_name == name:
                    return s
        return None

    def _find_enum(self, d_obj, enum_obj):
        enum_obj_name = enum_obj.__name__
        enum_pairs = [(enum["name"], enum) for enum in d_obj.enums]
        for name, e in enum_pairs:
            if enum_obj_name == name:
                return e
        #Handle the case where an enum is defined in another module
        for dep in d_obj.dependencies:
            enum_dep_pairs = [(enum["name"], enum) for enum in dep.enums]
            for name, e in enum_dep_pairs:
                if enum_obj_name == name:
                    return e
        return None

    def _parse_cpp(self):
        self._parse_c_cpp(True)

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
                    f_o.write("\n")
                for define in d.defines:
                    f_o.write("#define {}\t{}\n".format(define["name"], define["values"]))
                f_o.write("\n")
                for enum in d.enums:
                    implicit_enum_value = 0
                    if cpp:
                        # Enums are strongly typed in C++
                        f_o.write("enum {}:{} {{\n".format(enum["name"], self.data_type_map[enum["parent_cls"]]))
                    else:
                        # Typedef so we get some type checking in C
                        f_o.write("typedef enum {} {{\n".format(enum["name"]))

                    for pair in enum["values"]:
                        f_o.write("  {} = {},\n".format(pair[0], pair[1]))

                    if cpp:
                        f_o.write("};\n")
                    else:
                        f_o.write("}} {};\n".format(enum["name"]))

                    if cpp:
                        f_o.write('static_assert(sizeof({0}) == {1}, "Enum \'{0}\' has an incorrect size!");\n'.format(enum["name"], self._sizeof(enum["parent_cls"])))
                    else:
                        f_o.write("STATIC_ASSERT_PROJ(sizeof({0}) == {1}, INCORRECT_SIZE_{0});\n".format(enum["name"], self._sizeof(enum["parent_cls"])))
                    f_o.write("\n")

                for struct in d.structs:
                    # print("STRUCT:")
                    # pprint(struct)
                    struct_text_lines = []
                    struct_indentation = 2
                    if issubclass(struct['parent_cls'], ctypes.BigEndianStructure):
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
                        if "length" in field:
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
                
                for define in d.defines:
                    f_o.write("{} = ({})\n".format(define["name"], define["values"]))
                f_o.write("\n")

                for enum in d.enums:
                    f_o.write("class {}({}):\n".format(enum["name"], enum["parent_cls"]().__class__.__name__))
                    for pair in enum["values"]:
                        f_o.write("    {} = 0x{:X}\n".format(pair[0], pair[1]))
                    f_o.write("\n")

                for struct in d.structs:
                    parsed_structs = self._parse_struct(d, struct, False)
                    variable_len_struct = False
                    # Look ahead to see if we need to define this as a class factory instead of a plain class
                    if issubclass(struct["parent_cls"], ctypes.BigEndianStructure):
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
                        try:
                            f_o.write("              (\"{}\", {}),\n".format(line[0], line[1] + line[2]))
                        except:
                            print(line)
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
