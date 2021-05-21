## How to define interfaces

There are two basic interface elements that can be defines; structs and enums.
Structs define how data should be laid out when passed to an interface, and
enums assign meaningful names to status or control constants.

Because the interfaces are defined in pure Python, other interfaces can be included
using Python-style imports:
```python
from ctypes import *

import common_application_interface

temperature_app_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"nTS",
    "type":c_uint32},
    {"name":"nTemperature1",
    "type":c_uint16},
    {"name":"nTemperature2",
    "type":c_uint16},
  ]
}
```

### Structs
Structs define how data should be laid out when passed to an application.

Structs are defined as dictionaries, and they must contain a `struct_fields` list.
The members of the `struct_fields` list are also dictionaries, which must contain
the following mandatory fields:
- `name`: The text name of the field. If this is specified as `None`, and the `type`
of this field is set to a user-defined struct type, then that user-defined type
will automatically be fully-resolved in generated code.
- `type`: The type of the field. If the field should have a `stdint` type, then
the appropriate Python ctypes structure should be used (i.e. c_uint16). This
field may contain custom enums or structs that have already been defined.
Members of `struct_fields` may also optionally contain a `length` field. If the
`length` field is set equal to 0, then code will be generated for a variable-length
structure (C99 flexible array member). If the `length` field is greater than 0,
the field is specified as being an array of the specified type.


__NOTE:__ If `length` is specified for a struct which also has `name == None`,
then the `length` field is ignored.

Structs may optionally contain a `"big_endian": True` field. The presence of this
field will cause the generated struct code to either contain a language-specific
mechanism which declares the struct as being big endian, or will insert comments
noting that the struct is big endian if a language does not provide a mechanism
to enforce endianness.

#### Example
Structure declaration:
```python
temperature_app_stream_t = {
  "big_endian": True,
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"nTS",
    "type":c_uint32},
    {"name":"nTemperature1",
    "type":c_uint16,
    "length": 10},
    {"name":"nTemperature2",
    "type":c_uint16,
    "length":0},
  ]
}
```

Generated C code, showing the `common_application_interface._m2m2_app_data_stream_hdr_t` field
being fully resolved:
```c
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@  NOTE: THE FIELDS IN THIS STRUCTURE ARE BIG ENDIAN!  @@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
typedef struct _temperature_app_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint32_t  nTS;
  uint16_t  nTemperature1[10];
  uint16_t  nTemperature2[0];
} temperature_app_stream_t;
```

### Enums
Enums assign meaningful names to status or control constants.

Enums are defined as dictionaries, and they must contain the following mandatory fields:
- `type`: The type of the enum. This is used to force an enum to be a certain well
defined size (i.e. a single byte, two bytes, etc.)
- `enum_values`: A list of (_name_, _value_) pairs. The _value_ may be omitted, in
which case it will be determined automatically in the same way as a C/C++ enum.

#### Example
Enum declaration:
```python
M2M2_TEMPERATURE_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_TEMPERATURE_APP_CMD_LOWEST",        0x40),
    ("M2M2_TEMPERATURE_APP_CMD_SET_FS_REQ",     0x42),
    ("M2M2_TEMPERATURE_APP_CMD_SET_FS_RESP"),
    ]
}
```
Generated C code:
```c
typedef enum M2M2_TEMPERATURE_APP_CMD_ENUM_t {
  _M2M2_TEMPERATURE_APP_CMD_LOWEST = 64,
  M2M2_TEMPERATURE_APP_CMD_SET_FS_REQ = 66,
  M2M2_TEMPERATURE_APP_CMD_SET_FS_RESP = 67,
} M2M2_TEMPERATURE_APP_CMD_ENUM_t;
```
