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
            
            # split into chunks of 16 bytes
            chunk_size = 16
            total_chunks = (len(reg_locs) + chunk_size - 1) // chunk_size
            
            for chunk_idx in range(total_chunks):
                start = chunk_idx * chunk_size
                end = start + chunk_size
                chunk = reg_locs[start:end]
                # use upper
                chunk_name = (
                    f"{name_prefix.upper()}"
                    if total_chunks == 1
                    else f"{name_prefix.upper()}_{chunk_idx}"
                )
                chunk_str = ", ".join(chunk)
                lines.append(f'    {{"{chunk_name}", Parameter({{{chunk_str}}}, {default_value})}},')

    else:
        # look for nested subkeys if props is a dict
        if isinstance(props, dict):
            for subkey, subval in props.items():
                new_prefix = f"{name_prefix}_{subkey}"
                process_register(new_prefix, subval, lines)

def generate_header(input_yaml, data):
    """Generate the C++ header content for a given page."""
    lines = []
    lines.append(f'/* auto-generated LUT header from {input_yaml} */\n')
    lines.append("#pragma once\n")
    lines.append('#include "register_maps/register_maps_types.h"\n')
    lines.append("namespace econd {\n")

    # Loop through all blocks (e.g. ALIGNER, CHALIGNER)
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

    #lines.append("\nconst ParameterLUT PARAMETER_LUT = ParameterLUT::Mapping({")
    #for name in page_names:
    #    lines.append(f'  {{"{name}", {name}}},')
    #lines.append("});")
        
    lines.append("\nconst PageLUT PAGE_LUT = PageLUT::Mapping({")
    for name in page_names:
        lines.append(f'  {{"{name}", {name}}},')
    lines.append("});")

    lines.append("\n} // namespace econd\n")

    return "\n".join(lines)

def compile_registers(yaml_file, output_file):
    with open(yaml_file, "r") as f:
        data = yaml.safe_load(f)

    content = generate_header(yaml_file, data)
    with open(output_file, "w") as f:
        f.write(content)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 compile_registers.py <input.yaml> <output_file>")
        sys.exit(1)

    compile_registers(sys.argv[1], sys.argv[2])
