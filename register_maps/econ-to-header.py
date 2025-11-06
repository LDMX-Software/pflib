import sys
import yaml
from pathlib import Path

def count_bits(mask):
    """Count number of 1s in the binary representation of mask."""
    return bin(mask).count("1")

def safe_int(val):
    """Convert YAML value to int whether it's a string like '0x0398' or already an int."""
    if isinstance(val, str):
        return int(val, 16) if val.startswith("0x") else int(val)
    return int(val)

def format_cpp_int(value: int) -> str:
    """Append 'ULL' if value is larger than 32-bit signed int"""
    if value < 0 or value > 0x7FFFFFFF:
        return f'{value}ULL'
    else:
        return str(value)

def get_global_register_range(data):
    """
    Compute the minimum and maximum register addresses across all pages and registers
    in the YAML LUT.    
    """
    min_addr = None
    max_addr = None
    used_addresses = set()

    def update_range(address, mask, shift, size_byte):
        nonlocal min_addr, max_addr
        reg_locs = make_register_locations(address, mask, shift, size_byte)
        for loc in reg_locs:
            addr = int(loc.split("(")[1].split(",")[0], 16)
            used_addresses.add(addr)
            if min_addr is None or addr < min_addr:
                min_addr = addr
            if max_addr is None or addr > max_addr:
                max_addr = addr

    def process_node(props):
        if isinstance(props, dict):
            if "address" in props:
                address = int(props["address"])
                mask = int(props["param_mask"])
                shift = int(props["param_shift"])
                size_byte = props.get("size_byte", 1)
                update_range(address, mask, shift, size_byte)
            else:
                for subval in props.values():
                    process_node(subval)

    # loop over all pages and registers
    for page, groups in data.items():
        for group_name, registers in groups.items():
            if isinstance(registers, dict) and "address" in registers:
                process_node(registers)
            else:
                for reg_name, props in registers.items():
                    process_node(props)

    # compute unused addresses
    unused_addresses = []
    if min_addr is not None and max_addr is not None:
        full_range = set(range(min_addr, max_addr + 1))
        unused_addresses = sorted(full_range - used_addresses)
        
    print(f"Global register range: 0x{min_addr:04X} - 0x{max_addr:04X}")
    print(f"Unused addresses length: {len(unused_addresses)}")

    
def make_register_locations(address, mask, shift, size_byte):
    """Split a register into 8-bit chunks if it spans multiple bytes.
    syntax reminder: 
       RegisterLocation(reg, min_bit, n_bits):
     - reg = register 16-bit address
     - min_bit = param_shift 
     - n_bits = number of 1's in the mask for the chunk
    Any byte that contributes bits to the parameter gets n_bits > 0,
    unused bytes get n_bits = 0.

    Example: HEADER_MARKER (mask=0x01ff, shift=47, size_byte=7)
    - returns:
      RegisterLocation(0xf32, 0, 0)
      RegisterLocation(0xf31, 0, 0)
      RegisterLocation(0xf30, 0, 0)
      RegisterLocation(0xf2f, 0, 0)
      RegisterLocation(0xf2e, 0, 0)
      RegisterLocation(0xf2d, 0, 8)
      RegisterLocation(0xf2c, 0, 1)
    """
    reg_locs = []
    n_bits_total = bin(mask).count("1")
    if n_bits_total == 0:
        # if no bits used all bytes get n_bits=0
        return [f"RegisterLocation(0x{address + i:04x}, 0, 0)" for i in range(size_byte)]

    # determine which byte the parameter starts in
    start_byte_offset = shift // 8
    start_bit_offset = shift % 8

    # build the full list of bytes (from low to high address)
    for i in range(size_byte):
        curr_addr = address + i

        # compute bits in this byte if i is part of the parameter bytes
        param_byte_index = i - start_byte_offset
        if 0 <= param_byte_index <= (n_bits_total + start_bit_offset - 1) // 8:
            if param_byte_index == 0:
                bits_in_byte = min(8 - start_bit_offset, n_bits_total)
            elif param_byte_index == (n_bits_total + start_bit_offset - 1) // 8:
                bits_in_byte = (n_bits_total + start_bit_offset) % 8
                if bits_in_byte == 0:
                    bits_in_byte = 8
            else:
                bits_in_byte = 8
        else:
            # if not, use 0
            bits_in_byte = 0
            
        # shift for the byte: first byte uses start_bit_offset, others 0
        shift_for_byte = start_bit_offset if param_byte_index == 0 and bits_in_byte > 0 else 0

        #print("RegisterLocation ", hex(curr_addr), shift_for_byte, bits_in_byte)
        reg_locs.append(f"RegisterLocation(0x{curr_addr:04x}, {shift_for_byte}, {bits_in_byte})")

    return reg_locs

def process_register(name_prefix, props, lines, register_byte_lut = {}):
    """
    Process recursively a register or nested subregisters and append C++ lines.
    This will split big registers (>16 bytes) into multiple Parameters with suffixes (16 bytes is the max that lpGBT can handle).
    name_prefix: current register name prefix (e.g., 'USER_WORD')
    props: dict containing either address/mask/shift or nested subkeys
    lines: list of strings to append to
    """
    if isinstance(props, dict):
        if "address" in props:
            address = safe_int(props["address"])
            mask = safe_int(props["param_mask"])
            shift = safe_int(props["param_shift"])
            default_value = props.get("default_value", 0)
            size_byte = props.get("size_byte", 1)

            if address not in register_byte_lut: 
                register_byte_lut[address] = size_byte
            
            # generate all register locations and account for multi-byte registers
            #print(f"{name_prefix}: address={hex(address)}, mask={bin(mask).count('1')}, shift={shift}, size_byte={size_byte}")
            reg_locs = make_register_locations(address, mask, shift, size_byte)
            reg_locs_str = ", ".join(reg_locs)
            
            # split into chunks of 8 bytes (the lpGBT can do 16 byte operations, but keeping it to 8 bytes allows us to do keep 64-bit integers as default?)
            chunk_size = 8
            total_chunks = (len(reg_locs) + chunk_size - 1) // chunk_size
            for chunk_idx in range(total_chunks):
                start = chunk_idx * chunk_size
                end = start + chunk_size
                chunk = reg_locs[start:end]
                
                # compute bit offset (each reg_loc corresponds to 1 byte)
                bit_offset = chunk_idx * 8 * 8  # 8 bits/byte * 8 bytes = 64 bits per chunk
                chunk_bits = len(chunk) * 8
                chunk_mask = (1 << chunk_bits) - 1
                chunk_default = (default_value >> bit_offset) & chunk_mask
                
                chunk_name = (
                    f"{name_prefix.upper()}"
                    if total_chunks == 1
                    else f"{name_prefix.upper()}_{chunk_idx}"
                )
                chunk_str = ", ".join(chunk)
                lines.append(f'  the_map["{chunk_name}"] = Parameter({{{chunk_str}}}, {format_cpp_int(chunk_default)});')
                #print(chunk_str, hex(chunk_default))
        else:
            if isinstance(props, dict):
                for subkey, subval in props.items():
                    new_prefix = f"{name_prefix}_{subkey}"
                    process_register(new_prefix, subval, lines, register_byte_lut)        
    else:
        # look for nested subkeys if props is a dict
        if isinstance(props, dict):
            for subkey, subval in props.items():
                new_prefix = f"{name_prefix}_{subkey}"
                process_register(new_prefix, subval, lines, register_byte_lut)


def generate_header(input_yaml, data, econ_type):
    """Generate the C++ header content for a given page."""
    lines = []
    lines.append(f'/* auto-generated LUT header from {input_yaml} */\n')
    lines.append("#pragma once\n")
    lines.append('#include "register_maps/register_maps_types.h"\n')
    lines.append(f"namespace econ{econ_type}"+" {\n")

    register_byte_lut = {}
    
    # loop through all blocks (e.g. ALIGNER, CHALIGNER)
    page_names = []
    for page_name, groups in data.items():
        page_var = page_name.upper() # uppercase page name
        lines.append(f"static Page::Mapping get_{page_var}() {{")
        lines.append("  Page::Mapping the_map;")
        for group_name, registers in groups.items():
            # check if registers itself has an "address" key → it is a single register
            if isinstance(registers, dict) and "address" in registers:
                # single register (e.g. AlgoThreshold 0 in ECON-T)
                name_prefix = str(group_name)
                process_register(name_prefix, registers, lines, register_byte_lut)
            else:
                for reg_name, props in registers.items():
                    name_prefix = f"{group_name}_{reg_name}"
                    process_register(name_prefix, props, lines, register_byte_lut)
            
        page_names.append(page_var)
        lines.append("  return the_map;")
        lines.append(f"}} // get_{page_var}\n")
        lines.append(f"const Page {page_var} = get_{page_var}();\n")

    # print(register_byte_lut)
        
    lines.append("\nconst PageLUT PAGE_LUT = PageLUT::Mapping({")
    for name in page_names:
        lines.append(f'  {{"{name}", {name}}},')
    lines.append("});")

    lines.append("\nconst ParameterLUT PARAMETER_LUT = ParameterLUT::Mapping({")
    for name in page_names:
        lines.append(f'  {{"{name}", {{0, {name}}}}},')
    lines.append("});")
    
    lines.append("\n} //"+f" namespace econ{econ_type}\n")

    return "\n".join(lines)

def compile_registers(yaml_file, output_file):
    with open(yaml_file, "r") as f:
        data = yaml.safe_load(f)

    econ_type =	"d" if "ECOND" in yaml_file else "t"
    if "test" in yaml_file:
        econ_type += "_test"
    content = generate_header(yaml_file, data, econ_type)
    with open(output_file, "w") as f:
        f.write(content)
        
    #get_global_register_range(data)
        
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 econ-to-header.py <input.yaml> <output_file>")
        sys.exit(1)

    compile_registers(sys.argv[1], sys.argv[2])
