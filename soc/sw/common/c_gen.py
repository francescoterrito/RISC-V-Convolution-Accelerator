# Copyright 2024 Politecnico di Torino.
# Copyright and related rights are licensed under the Solderpad Hardware
# License, Version 2.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://solderpad.org/licenses/SHL-2.0. Unless required by applicable law
# or agreed to in writing, software, hardware and materials distributed under
# this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.
#
# File: c_gen.py
# Author(s):
#   Michele Caon
# Date: 22/11/2024
# Description: Convert numpy data and other information to C arrays.

# Write a C header file with array definitions for the input matrix, the output
# matrix, and the instruction stream.

import os
import sys
import numpy as np

class CFileGen:
    """
    A class for generating C code files containing binary data, matrices, and code.

    Attributes:
        binaries (List[Tuple[str, str]]): A list of binary files to dump in the generated C file.
        codes (List[Tuple[str, np.ndarray]]): A list of commands or instruction sequences to include in the generated C file.
        input_matrices (List[Tuple[str, np.ndarray]]): A list of input matrices to include in the generated C file.
        output_matrices (List[Tuple[str, np.ndarray]]): A list of output matrices to include in the generated C file.
        macros (List[Tuple[str, int, Optional[str]]]): A list of macros to include in the generated C file.
        macros_hex (List[Tuple[str, str, Optional[str]]]): A list of string macros to include in the generated C file (e.g., hex values).
        macros_raw (List[Tuple[str, str, Optional[str]]]): A list of macros in raw format to include in the generated C file.
        attributes (List[str]): A list of C attributes to apply to the generated C arrays.
    """
    
    def __init__(self, base_name: str) -> None:
        self.header_file = f"{base_name}.h"
        self.source_file = f"{base_name}.c"
        self.comments = []
        self.binaries = []
        self.codes = []
        self.input_matrices = []
        self.output_matrices = []
        self.macros = []
        self.macros_hex = []
        self.macros_raw = []
        self.attributes = []

    # Add a comment
    def add_comment(self, comment: str) -> None:
        self.comments.append(('', 0, comment))

    # Add a new binary file
    def add_binary(self, name: str, file: str) -> None:
        self.binaries.append((name, file))

    # Add a new list of commands
    def add_code(self, name: str, matrix: np.ndarray) -> None:
        self.codes.append((name, matrix))

    # Add a new input matrix definition
    def add_input_matrix(self, name: str, matrix: np.ndarray) -> None:
        self.input_matrices.append((name, matrix))

    # Add a new output matrix definition
    def add_output_matrix(self, name: str, matrix: np.ndarray) -> None:
        self.output_matrices.append((name, matrix))

    # Add a macro
    def add_macro(self, name: str, value: int, comment: str = None) -> None:
        self.macros.append((name, value, comment))
    def add_macro_hex(self, name: str, value: str, comment: str = None) -> None:
        self.macros_hex.append((name, value, comment))
    def add_macro_raw(self, name: str, value: str, comment: str = None) -> None:
        self.macros_raw.append((name, value, comment))
    def add_macros_from_source(self, src_file: str) -> None:
        # Open source file and parse macros
        with open(src_file, 'r') as src:
            for line in src:
                if line.startswith('#define'):
                    tokens = line.split()
                    name = tokens[1]
                    value = int(tokens[2], 0)
                    comment = ' '.join(tokens[3:]) if len(tokens) > 3 else None
                    self.add_macro_raw(name, value, comment)

    # Define variable attributes
    def add_attribute(self, value: str) -> None:
        self.attributes.append(value)

    # Convert signed dtype to unsigned dtype
    def signed2unsigned(self, dtype: np.dtype) -> np.dtype:
        return {
            'int8': np.uint8,
            'int16': np.uint16,
            'int32': np.uint32,
        }[str(dtype)]
    
    # Convert numpy dtype to C type
    def dtype_to_ctype(self, dtype: np.dtype) -> str:
        return {
            'int8': 'int8_t',
            'int16': 'int16_t',
            'int32': 'int32_t',
            'uint8': 'uint8_t',
            'uint16': 'uint16_t',
            'uint32': 'uint32_t',
        }[str(dtype)]
        
    # Format binary file content as C array
    def format_binary(self, name: str, file: str) -> str:
        # Read binary file
        with open(file, 'rb') as f:
            content = f.read()

        # Determine the length of the data
        data_size = len(content)
        data_len = data_size // 4
        if (data_size % 4) != 0:
            # pad with zeros
            content += b'\x00' * (4 - (data_size % 4))
            data_len += 1

        # Write C data content
        array_contents = f"uint32_t {name}[] = {{\n"
        for i in range(data_len):
            element = int.from_bytes(content[i * 4 : (i + 1) * 4], byteorder='little')
            array_contents += f"    0x{element:08X}"
            if i != data_len - 1:
                array_contents += ",\n"
        array_contents += "\n};\n"
        return array_contents
        
    # Format matrix size macros
    def format_matrix_size(self, matrix: np.ndarray, name: str) -> str:
        size_contents = f"#define {name.upper()}_SIZE {matrix.size * matrix.itemsize}\n"
        size_contents += f"#define {name.upper()}_ROWS {matrix.shape[0]}\n"
        size_contents += f"#define {name.upper()}_COLS {matrix.shape[1] if len(matrix.shape) > 1 else 1}\n"
        return size_contents
    
    # Format code size macros
    def format_code_size(self, code: str, name: str) -> str:
        size_contents = f"#define {name.upper()}_SIZE {len(code)*4}\n"
        return size_contents

    # Format matrix for C
    def format_matrix(self, matrix: np.ndarray, name: str) -> str:
        # Determine the number of bits based on the dtype
        dtype: np.dtype = matrix.dtype
        num_bits = dtype.itemsize * 8

        array_ctype = self.dtype_to_ctype(dtype)
        utype = self.signed2unsigned(dtype)
        matrix = matrix.astype(utype)

        # Flatten the array in C (row-major) order
        flat_array = matrix.flatten(order='C')

        # Convert the elements to 2's complement hexadecimal strings
        hex_values = [f"{element:#0{2+num_bits//4}x}" for element in flat_array]

        # Define a recursive function to generate the structure comments
        def generate_structure_comments(array, index=0, indent_level=0, is_top_level=False):
            lines = []
            indent = '    ' * indent_level

            if array.ndim == 1:
                # Base case: 1D array
                line = indent + '/* { */ ' + ', '.join(hex_values[index:index+array.size]) + ' /* } */'
                index += array.size
                lines.append(line)
            else:
                # Recursive case: Multi-dimensional array
                num_subarrays = len(array)
                # Add opening comment if not top-level
                if not is_top_level:
                    lines.append(indent + '/* { */')
                for idx, subarray in enumerate(array):
                    # Process subarray
                    sub_lines, index = generate_structure_comments(
                        subarray, index, indent_level + 1, is_top_level=False)
                    lines.extend(sub_lines)
                    # Add comma to the last line of sub_lines if not last subarray
                    if idx < num_subarrays - 1:
                        lines[-1] += ','
                # Add closing comment if not top-level
                if not is_top_level:
                    lines.append(indent + '/* } */')
                else:
                    # At top level, add comma after each subarray except the last
                    if idx < num_subarrays - 1:
                        lines[-1] += ','
            return lines, index

        # Generate the lines with structure comments
        lines, _ = generate_structure_comments(matrix, is_top_level=True)

        # Build the final string
        matrix_contents = f"{array_ctype} {name}[]"
        if len(self.attributes) > 0:
            matrix_contents += f" __attribute__(({','.join(self.attributes)}))"
        matrix_contents += ' = {\n'
        matrix_contents += '\n'.join(lines)
        matrix_contents += '\n};\n\n'

        return matrix_contents


    # Format code for C
    def format_code(self, code: str, name: str) -> str:
        # Format the array
        code_contents = f"uint32_t {name}[] "
        if len(self.attributes) > 0:
            code_contents += f"__attribute__(({','.join(self.attributes)})) "
        code_contents += "= {"
        for i, insn in enumerate(code):
            if i % 8 == 0:
                code_contents += "\n    "
            code_contents += f"{insn:>10}"
            if i < len(code) - 1:
                code_contents += ", "
        code_contents += "\n};\n"
        return code_contents

    # Write the header file
    def gen_header(self, header_macro: str = None) -> str:

        if header_macro is not None:
            header_contents = f"// Auto-generated by {os.path.basename(__file__)}\n\n"
            # Header guard
            header_contents += f'#ifndef {header_macro}\n#define {header_macro}\n\n'    
            # Include stdint.h
            header_contents += "#include <stdint.h>\n\n"

        # Comments
        if len(self.comments) > 0:
            for name, value, comment in self.comments:
                header_contents += f"// {comment}\n"
            header_contents += '\n'

        # Macros
        if len(self.macros) > 0 or len(self.macros_hex) > 0 or len(self.macros_raw) > 0:
            header_contents += "// Macros\n"
            header_contents += "// ------\n"
        for name, value, comment in self.macros:
            header_contents += f"#define {name.upper()} {value}"
            if comment is not None:
                header_contents += f" // {comment}\n"
            else:
                header_contents += '\n'
        for name, value, comment in self.macros_hex:
            header_contents += f"#define {name.upper()} 0x{value:08X}"
            if comment is not None:
                header_contents += f" // {comment}\n"
            else:
                header_contents += '\n'
        for name, value, comment in self.macros_raw:
            header_contents += f"#define {name} {value}"
            if comment is not None:
                # Check if the comment starts with // or /*
                if comment.startswith('//') or comment.startswith('/*'):
                    header_contents += f" {comment}\n"
                else:
                    header_contents += f" // {comment}\n"
            else:
                header_contents += '\n'
        if len(self.macros) > 0 or len(self.macros_hex) > 0 or len(self.macros_raw) > 0:
            header_contents += '\n'

        # Macros with array sizes
        if len(self.binaries) > 0:
            header_contents += "// Binary size\n"
            header_contents += "// -----------\n"
            for name, file in self.binaries:
                file_size = os.path.getsize(file)
                if file_size % 4 != 0:
                    file_size += 4 - (file_size % 4)
                header_contents += f"#define {name.upper()}_SIZE {file_size}\n"
            header_contents += '\n'

        if len(self.input_matrices) > 0:
            header_contents += "// Input matrix size\n"
            for name, matrix in self.input_matrices:
                header_contents += self.format_matrix_size(matrix, name)
            header_contents += '\n'

        if len(self.output_matrices) > 0:
            header_contents += '// Output matrix size\n'
            for name, matrix in self.output_matrices:
                header_contents += self.format_matrix_size(matrix, name)
            header_contents += '\n'

        if len(self.codes) > 0:
            header_contents += '// Code size\n'
            for name, code in self.codes:
                header_contents += self.format_code_size(code, name)
            header_contents += '\n'

        # Write binary files
        if len(self.binaries) > 0:
            header_contents += "// Binary files\n"
            header_contents += "// ------------\n"
            for name, _ in self.binaries:
                header_contents += f"extern uint32_t {name}[];\n"
            header_contents += '\n'

        # Write code arrays
        if len(self.codes) > 0:
            header_contents += "// Code\n"
            header_contents += "// ----\n"
            for name, _ in self.codes:
                header_contents += f"extern uint32_t {name}[];\n"
            header_contents += '\n'

        # Write input matrices
        if len(self.input_matrices) > 0:
            header_contents += "// Input matrices\n"
            header_contents += "// --------------\n"
            for name, matrix in self.input_matrices:
                dtype: np.dtype = matrix.dtype
                array_ctype = self.dtype_to_ctype(dtype)
                header_contents += f"extern {array_ctype} {name}[];\n"
            header_contents += '\n'

        # Write output matrices
        if len(self.output_matrices) > 0:
            header_contents += "// Output matrices\n"
            header_contents += "// ---------------\n"
            for name, matrix in self.output_matrices:
                dtype: np.dtype = matrix.dtype
                array_ctype = self.dtype_to_ctype(dtype)
                header_contents += f"extern {array_ctype} {name}[];\n"
            header_contents += '\n'

        if header_macro is not None:
            header_contents += f"#endif // {header_macro}\n"

        # Return the header contents
        return header_contents
    
    # Write the source file
    def gen_source(self) -> str:
        source_contents = f"// Auto-generated by {os.path.basename(__file__)}\n\n"

        # Include stdint.h
        source_contents += "#include <stdint.h>\n"

        # Include the header file
        source_contents += f"#include \"{self.header_file}\"\n\n"
        
        # Write binary files
        if len(self.binaries) > 0:
            source_contents += "// Binary files\n"
            source_contents += "// ------------\n"
            for name, file in self.binaries:
                source_contents += self.format_binary(name, file)
            source_contents += '\n'

        # Write code arrays
        if len(self.codes) > 0:
            source_contents += "// Code\n"
            source_contents += "// ----\n"
            for name, code in self.codes:
                source_contents += self.format_code(code, name)
            source_contents += '\n'

        # Write input matrices
        if len(self.input_matrices) > 0:
            source_contents += "// Input matrices\n"
            source_contents += "// --------------\n"
            for name, matrix in self.input_matrices:
                source_contents += self.format_matrix(matrix, name)

        # Write output matrices
        if len(self.output_matrices) > 0:
            source_contents += "// Output matrices\n"
            source_contents += "// ---------------\n"
            for name, matrix in self.output_matrices:
                source_contents += self.format_matrix(matrix, name)

        # Return the header contents
        return source_contents

    def write_header(self, directory: str) -> None:
        # Header file path
        header_path = os.path.join(directory, self.header_file)
        header_base = os.path.basename(header_path)
        header_macro = header_base.upper().replace('.', '_') + '_'

        # Generate header
        header_contents = self.gen_header(header_macro)

        # Write header file
        print(f"Writing header file '{header_path}'...")
        with open(header_path, 'w') as header_file:
            header_file.write(header_contents)

    def append_header(self, file, header_macro: str = None):
        # Generate header
        header_contents = self.gen_header(header_macro)

        # Write header file
        file.write(header_contents)

    def write_source(self, directory: str) -> None:
        # Source file path
        source_path = os.path.join(directory, self.source_file)

        # Generate header
        source_contents = self.gen_source()

        # Write source file
        print(f"Writing source file '{source_path}'...")
        with open(source_path, 'w') as source_file:
            source_file.write(source_contents)

if __name__ == "__main__":
    # Check the number of arguments
    if len(sys.argv) < 3:
        print("Usage: python c_gen.py <base_name> <bin_file> [<src_file>]")
        sys.exit(1)

    # Parse arguments
    base_name = sys.argv[1]
    bin_file = sys.argv[2]
    src_file = sys.argv[3] if len(sys.argv) > 3 else None

    # Determine kernel name
    kernel_name = os.path.splitext(os.path.basename(base_name))[0]
    out_dir = os.path.dirname(base_name)
    base_name = f"{kernel_name}.h"

    # Generate C header from input binary file
    header_gen = CFileGen(kernel_name)
    header_gen.add_binary(kernel_name, bin_file)

    # Add macros from source file
    if src_file is not None:
        print(f"Adding macros from source file '{src_file}'...")
        header_gen.add_macros_from_source(src_file)

    # Write header file
    header_gen.write_header(out_dir)
    header_gen.write_source(out_dir)
