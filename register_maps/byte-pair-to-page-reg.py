"""convert R0/R1 byte pair addresses into Page/Register addresses"""

import yaml
import argparse
from dataclasses import dataclass, asdict
from typing import List, Dict
from pathlib import Path


def byte_pair_to_subblock_register(r0, r1):
    fulladdr = (r1 << 8) | r0
    reg = fulladdr & 0b11111
    subblock = fulladdr >> 5
    return subblock, reg


@dataclass
class RegisterLocationInSubBlock:
    """A specific location within a register in a subblock"""
    register: int
    min_bit: int
    n_bits: int

    @classmethod
    def from_swamp_register(cls, regnode):
        _subblock, register = byte_pair_to_subblock_register(regnode['R0'], regnode['R1'])
        # unpack mask into two human-helpful pieces of information
        # string operations are annoying and slow but reliable and understandable
        #   the min bit is the first index from the end of the string with a '1'
        #   the number of bits is the number of '1'
        bits = f'{regnode["reg_mask"]:b}'
        return cls(
            register = register,
            min_bit = len(bits)-bits.rindex('1')-1,
            n_bits = bits.count('1')
        )

    def to_cpp(self):
        """Convert this register location into the C++ equivalent struct

        The `RegisterLocation` struct is defined in register_maps/register_maps_types.h
        and included before any of the implementation headers that are generated by this script are included.
        """
        return f'RegisterLocation({self.register}, {self.min_bit}, {self.n_bits})'


@dataclass
class Parameter:
    """All parameters reside within a single page but potentially across multiple registers.

    The manual defines a default value for the parameter (which may or may not be respected by the chips),
    so we carry that around along with the list of RegisterLocationInSubBlock defining the parameter's bits
    in order from least significant bit up to most significant bit.
    """
    
    bit_locations: List[RegisterLocationInSubBlock]
    default_value: str

    def __post_init__(self):
        if len(self.bit_locations) == 0:
            raise ValueError('A parameter cannot exist without any bit locations.')

    @classmethod
    def from_swamp_parameter(cls, parameter_node):
        # the defval_mask is relative to the register it is in
        # so we construct the default value after deducing the n_bits and min_bits
        locations =  [
            RegisterLocationInSubBlock.from_swamp_register(regnode)
            for i_chunk, regnode in parameter_node.items()
        ]
        defval = 0
        curr_min_bit = 0
        for regnode, location in zip(parameter_node.values(), locations):
            chunk = ((regnode['defval_mask'] >> location.min_bit) & ((1 << location.n_bits) - 1))
            defval |= (chunk << curr_min_bit)
            curr_min_bit += location.n_bits
        # store the default value as a string in binary, padded out to the full length
        # again, hopefully making it easier for comparison to the manual
        bit_rep = '0b'+f'{defval:0b}'.zfill(curr_min_bit) if defval != 0 else '0'
        return cls(bit_locations = locations, default_value = bit_rep)


    def to_cpp(self):
        """Convert this parameter into its C++ representation

        The `Parameter` and `RegisterLocation` structs are defined in register_maps/register_maps_types.h
        before any of the implementation headers are included.
        """
        return 'Parameter({'+', '.join(l.to_cpp() for l in self.bit_locations)+'}, '+self.default_value+')'


@dataclass
class SubBlock:
    """A subblock has an address and a type specifying its parameters"""
    address: int
    type: str


parser = argparse.ArgumentParser()
parser.add_argument('input', help='input yaml file with R0/R1 addresses')
parser.add_argument('output', type=Path, help='output file with page/register addresses')
parser.add_argument('--no-intermediate-yaml', action='store_true', help='do not dump intermediate page/register addresses into a yaml for comparison. Used in the CMakeLists.txt to avoid extraneous files.')
parser.add_argument('--namespace', help='namespace to wrap LUTs with, default is stem of output file') 
args = parser.parse_args()

if args.namespace is None:
    args.namespace = args.output.stem

with open(args.input) as f:
    byte_pair_map = yaml.safe_load(f)

# the SWAMP block names sometimes correspond with the
# names of the subblock types in the manual (e.g. "Top", "DigitalHalf", "ReferenceVoltage")
# but sometimes don't (e.g. "ch" instead of "ChannelWise")
# I want to translate them so the page names inside of pflib can stay the same
# names (which are closer to those listed in the manual tables
blockname_to_subblock_type = {
    'ch': 'CHANNELWISE',
    'cm': 'CHANNELWISE',
    'calib': 'CHANNELWISE',
    'HalfWise': 'CHANNELWISE',
    'Top' : 'TOP',
    'DigitalHalf': 'DIGITALHALF',
    'GlobalAnalog': 'GLOBALANALOG',
    'MasterTdc': 'MASTERTDC',
    'ReferenceVoltage': 'REFERENCEVOLTAGE'
}
subblock_types = {}
subblocks = {}
for block_name, block_node in byte_pair_map.items():
    subblock_typename = blockname_to_subblock_type.get(block_name, block_name)
    for subblock_id, subblock_node in block_node.items():
        subblock_parameters = {
            name.upper(): Parameter.from_swamp_parameter(node)
            for name, node in subblock_node.items()
        }
        if block_name not in subblock_types:
            subblock_types[subblock_typename] = subblock_parameters
        else:
            if subblock_types[subblock_typename] != subblock_parameters:
                raise ValueError(f'Block {block_name}_{subblock_id} labeled with subblock type {subblock_typename} has different parameters than previous subblocks with the same type.')
        
        subblock_addresses = set(
            byte_pair_to_subblock_register(node['R0'], node['R1'])[0]
            for parameter in subblock_node.values()
            for node in parameter.values()
        )
        if len(subblock_addresses) != 1:
            raise ValueError(f'Subblock {block_name}_{subblock_id} does not have exactly one address! {subblock_addresses}')
        subblock_name = block_name.upper()
        # skip adding the index number to the TOP page since
        # there is only one of them
        if subblock_name != 'TOP':
            subblock_name += f'_{subblock_id}'
        subblocks[subblock_name] = SubBlock(
            address = subblock_addresses.pop(),
            type = subblock_typename
        )

# sort the parameters by their first register to help in comparison to the manual
subblock_types = {
    name: {
        k: parameters[k]
        for k in sorted(
             parameters.keys(),
             key = lambda k: parameters[k].bit_locations[0].register
        )
    }
    for name, parameters in subblock_types.items()
}

# on new builds, the directory we are writing the header to may not exist yet
args.output.parent.mkdir(exist_ok=True, parents=True)

# yaml dump for easier comparison to manual
if not args.no_intermediate_yaml:
    with open(args.output.with_suffix('.yaml'), 'w') as f:
        yaml.safe_dump(
            {
                'subblocks': {
                    name: asdict(subblock)
                    for name, subblock in subblocks.items()
                },
                'subblock_types': {
                    name: { param : asdict(val) }
                    for name, parameters in subblock_types.items()
                    for param, val in parameters.items()
                }
            },
            f,
            sort_keys=False # our order is intentional, do not do dictionary sort
        )


# C++ dump for inclusion in pflib
with open(args.output.with_suffix('.h'), 'w') as f:
    f.write(f'/* auto-generated LUT header from {args.input} */\n\n')
    f.write('#pragma once\n\n')
    # the types PAGE_TYPE, PAGE_LUT_TYPE, and PARAMETER_LUT_TYPE are defined
    # in register_maps/register_maps_types.h
    f.write('#include "register_maps/register_maps_types.h"\n\n')
    f.write('namespace %s {\n\n'%(args.namespace))
    for name, parameters in subblock_types.items():
        f.write('const Page %s = Page::Mapping({\n'%(name))
        f.write(',\n'.join(
            '  {"%s", %s }'%(parameter_name, parameter_spec.to_cpp())
            for parameter_name, parameter_spec in parameters.items()
        ))
        f.write('\n});\n\n')

    f.write('const PageLUT PAGE_LUT = PageLUT::Mapping({\n')
    f.write(',\n'.join(
        '  {"%s", %s }'%(name, name)
        for name in subblock_types
    ))
    f.write('\n});\n\n')

    f.write('const ParameterLUT PARAMETER_LUT = ParameterLUT::Mapping({\n')
    f.write(',\n'.join(
        '  {"%s", { %d, %s }}'%(name, subblock.address, subblock.type)
        for name, subblock in subblocks.items()
    ))
    f.write('\n});\n\n')
    f.write('} // namespace %s\n'%(args.namespace))


