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
     - min_bit = param_shift for the first chunk, 0 for others
     - n_bits = number of 1's in the mask for the chunk
    """
    reg_locs = []
    remaining_mask = mask
    curr_addr = address
    first_chunk = True

    for i in range(size_byte):
        # take the lowest 8 bits for this byte
        chunk_mask = remaining_mask & 0xFF
        n_bits = bin(chunk_mask).count("1")
        loc_shift = shift if first_chunk else 0
        reg_locs.append(f"RegisterLocation(0x{curr_addr:04x}, {loc_shift}, {n_bits})")

        # shift the remaining mask right by 8 bits to remove the bits we've already processed
        remaining_mask >>= 8
        # address increments by 1 byte
        curr_addr += 1
        # only the first chunk uses the original shift
        first_chunk = False

        # break the loop if no bits are left to process
        if remaining_mask == 0:
            break
    return reg_locs

def process_register(name_prefix, props, lines):
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

            # generate all register locations and account for multi-byte registers
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
                lines.append(f'    {{"{chunk_name}", Parameter({{{chunk_str}}}, {format_cpp_int(chunk_default)})}},')
                #print(chunk_str, hex(chunk_default))
    else:
        # look for nested subkeys if props is a dict
        if isinstance(props, dict):
            for subkey, subval in props.items():
                new_prefix = f"{name_prefix}_{subkey}"
                process_register(new_prefix, subval, lines)

def generate_header(input_yaml, data, econ_type):
    """Generate the C++ header content for a given page."""
    lines = []
    lines.append(f'/* auto-generated LUT header from {input_yaml} */\n')
    lines.append("#pragma once\n")
    lines.append('#include "register_maps/register_maps_types.h"\n')
    lines.append(f"namespace econ{econ_type}"+" {\n")

    # loop through all blocks (e.g. ALIGNER, CHALIGNER)
    page_names = []
    for page_name, groups in data.items():
        page_var = page_name.upper() # uppercase page name
        lines.append(f"const Page {page_var} = Page::Mapping({{")
        for group_name, registers in groups.items():
            # check if registers itself has an "address" key → it is a single register
            if isinstance(registers, dict) and "address" in registers:
                # single register (e.g. AlgoThreshold 0 in ECON-T)
                name_prefix = str(group_name)
                process_register(name_prefix, registers, lines)
            else:
                for reg_name, props in registers.items():
                    name_prefix = f"{group_name}_{reg_name}"
                    process_register(name_prefix, props, lines)
            
        page_names.append(page_var)
        lines.append("  });")

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
