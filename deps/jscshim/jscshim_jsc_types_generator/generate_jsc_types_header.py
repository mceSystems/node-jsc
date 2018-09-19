"""
This source code is licensed under the terms found in the LICENSE file in 
node-jsc's root directory.
"""

"""
generate_jsc_types_header.py

In order to use JSC types in v8.h (declarations only), without requiring users to include JSC headers, we'll generate a header
with "fake\placeholder" types, with placeholder object just having the same size as the original JSC type.
For example, defining:
	namespace JSC {
		class JSObject { private: char m_placeHolder[32]; };
	}
will allow us to define JSObject objects in v8.h (without using any of it's members, which will be done
in jscshim), not just as pointers (which will force us to dynamically allocate them). See the top comments
in v8.h for more information.

To get the size of the objects we need on the target platform, we need to compile some code doing "sizeof"
on objects, and extract it. Since we might be cross compiling, when can't just build an executable and run it.
Thus, we'll have the executable (jscshim_jsc_types_generator) store them in a structure we could easily 
find and parse from here. Basically, jscshim_jsc_types_generator contains the following declarations:
	#define MAGIC "!JSCSHIM_JSC_TYPES_START!"
	#define MAX_TYPE_NAME_SIZE (120)

	PACK_STRUCT(struct GeneratorTypesData
	{
		PACK_STRUCT(struct JSCType
		{
			char declaration[MAX_TYPE_NAME_SIZE];
			unsigned int size;
		})

		char m_magic[sizeof(MAGIC)];
		JSCType m_types[];
	})

	...
	const GeneratorTypesData SHIM_JSC_TYPES = {
		MAGIC,
		{
			...
		}
	};

Thus, we'll perform the following steps to generate our header:
1. Search for the "!JSCSHIM_JSC_TYPES_START!" "magic", indicating the beginning of SHIM_JSC_TYPES.
2. Parse each JSCType struct instance and extract the type declaration and size (until we reach an empty one).
   Note that we use the type "declaration" rather then just it's name to allow different 
3. Generate the "placeholders" definitions. Note that "empty" types are allowed, thus we support
   zero sized types (we'll create an "empty" type definintion).
4. Based on the generated deinitions, generate the final header. We'll use a template file, stored in 
   our directory.

This was inspired by how Webkit generates it's interpreter code, where a binary called LLIntOffsetsExtractor
containing every size\offset\value needed is built, and later parsed by a script (asm.rb).
"""
import sys
import os
import re
import struct

# extract_header.py <jscshim_jsc_types_generator compiled binary path> <generated header path>
CMD_ARG_COUNT = 3

MAGIC_STAMP_START = "!JSCSHIM_JSC_TYPES_START!"

# See "JSCType" type above
JSC_TYPE_STRUCT_FORMAT = "!120sI"
JSC_TYPE_STRUCT_SIZE = struct.calcsize(JSC_TYPE_STRUCT_FORMAT)

NON_EMPTY_TYPE_DEFINITION_LINE_FORMAT = "\t{0} {{ private: char m_placeHolder[{1}]; }};"
EMPTY_TYPE_DEFINITION_LINE_FORMAT = "\t{0} {{ }};"

TEMPLATE_FILE_NAME = "template.h"

def strip_string_trailing_null_bytes(s):
	return s[:s.find('\0')]

def extract_jsc_types(binary_file_data):
	types_start = binary_file_data.find(MAGIC_STAMP_START)
	if (types_start < 0):
		print("Invalid binary file")
		sys.exit()
	 
	# Skip the magic plus it's null terminator
	types_start += len(MAGIC_STAMP_START) + 1

	types = []
	while True:
		type_declaration, type_size = struct.unpack_from(JSC_TYPE_STRUCT_FORMAT, binary_file_data, types_start)
		type_declaration = strip_string_trailing_null_bytes(type_declaration)
		if 0 == len(type_declaration):
			break

		types.append((type_declaration, type_size))
		types_start += JSC_TYPE_STRUCT_SIZE

	return types

def generate_types_definition(types):
	lines = []
	for (type_declaration, type_size) in types:
		if 0 == type_size:
			lines.append(EMPTY_TYPE_DEFINITION_LINE_FORMAT.format(type_declaration))
		else:
			lines.append(NON_EMPTY_TYPE_DEFINITION_LINE_FORMAT.format(type_declaration, type_size))

	return "\n".join(lines)

def generate_header(types_definition):
	current_script_dir = os.path.dirname(os.path.realpath(__file__))
	template_file_path = os.path.join(current_script_dir, TEMPLATE_FILE_NAME)

	with open(template_file_path, "r") as template_file:
		template_data = template_file.read()
		return template_data.format(types_definition)

def main():
	if CMD_ARG_COUNT != len(sys.argv):
		print("Invalid usage, expected: extract_header.py <jscshim_jsc_types_generator binary path> <generated header path>")
		sys.exit()

	input_binary_file_path = sys.argv[1]
	output_header_file_path = sys.argv[2]
	print("Extracting types from {} to {}".format(input_binary_file_path, output_header_file_path))

	# Read the entire compiled binary file (it's pretty small so we'll read it's
	# entire contents at once for now) 
	binary_file_data = None
	with open(input_binary_file_path, "rb") as input_binary_file:
		binary_file_data = input_binary_file.read()

	# Extract the type declarations and sizes from the binary file
	types = extract_jsc_types(binary_file_data)

	# Generate the full header contents based on the definitions and template file
	types_definition = generate_types_definition(types)
	generated_header = generate_header(types_definition)

	# Finally, save the generated header file
	with open(output_header_file_path, "w") as output_header_file:
		output_header_file.write(generated_header)

	print("DONE")
  
if __name__== "__main__":
  main()