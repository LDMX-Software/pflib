"""convert R0/R1 byte pair addresses into Page/Register addresses"""

import yaml
import argparse
from dataclasses import dataclass, asdict
from typing import List, Dict


def byte_pair_to_page_register(r0, r1):
    fulladdr = (r1 << 8) | r0
    reg = fulladdr & 0b11111
    page = fulladdr >> 5
    return page, reg


@dataclass
class RegisterLocationInPage:
    """A specific location within a register in a page"""
    register: int
    min_bit: int
    n_bits: int

    @classmethod
    def from_swamp_register(cls, regnode):
        _page, register = byte_pair_to_page_register(regnode['R0'], regnode['R1'])
        # unpack mask into two human-helpful pieces of information
        # string operations are annoying/slow but reliable to understand
        bits = f'{regnode["reg_mask"]:b}'
        return cls(
            register = register,
            min_bit = len(bits)-bits.rindex('1')-1,
            n_bits = bits.count('1')
        )


@dataclass
class Parameter:
    """All parameters reside within a single page but potentially across multiple registers.

    The manual defines a default value for the parameter (which may or may not be respected by the chips),
    so we carry that around along with the list of RegisterLocationInPage defining the parameter's bits
    in order from least significant bit up to most significant bit.
    """
    
    bit_locations: List[RegisterLocationInPage]
    default_value: int

    @classmethod
    def from_swamp_parameter(cls, parameter_node):
        # TODO deduce the default value from the defval_mask
        return cls(
            bit_locations = [
                RegisterLocationInPage.from_swamp_register(regnode)
                for i_chunk, regnode in parameter_node.items()
            ],
            default_value = 0
        )


@dataclass
class PageType:
    """A page type is a specific set of parameters"""
    parameters: Dict[str,Parameter]


@dataclass
class Page:
    id: int
    type: str


def dict_diff(d1, d2):
    k1 = set(d1.keys())
    k2 = set(d2.keys())
    added = k2 - k1
    removed = k1 - k2
    shared = k1.intersection(k2)
    modified = {k: (d1[k], d2[k]) for k in shared if d1[k] != d2[k]}
    same = set(k for k in shared if d1[k] == d2[k])
    return added, removed, modified, same


parser = argparse.ArgumentParser()
parser.add_argument('input', help='input yaml file with R0/R1 addresses')
parser.add_argument('output', help='output yaml file with page/register addresses')
args = parser.parse_args()

with open(args.input) as f:
    byte_pair_map = yaml.safe_load(f)

page_types = {}
pages = {}
for block_name, block_node in byte_pair_map.items():
    for subblock_id, subblock_node in block_node.items():
        subblock_parameters = {
            name: Parameter.from_swamp_parameter(node)
            for name, node in subblock_node.items()
        }
        if block_name not in page_types:
            page_types[block_name] = PageType(parameters = subblock_parameters)
        else:
            if page_types[block_name].parameters != subblock_parameters:
                raise ValueError
        
        page_ids = set(
            byte_pair_to_page_register(node['R0'], node['R1'])[0]
            for parameter in subblock_node.values()
            for node in parameter.values()
        )
        if len(page_ids) > 1:
            raise ValueError
        pages[f'{block_name}_{subblock_id}'] = Page(id = page_ids.pop(), type = block_name)


with open(args.output, 'w') as f:
    yaml.safe_dump(
        {
            'pages': { name: asdict(page) for name, page in pages.items() },
            'page_types': { name: asdict(type) for name, type in page_types.items() }
        },
        f
    )

#parameter_spec = {}
#for block_name, block in page_reg_map.items():
#    parameter_spec[block_name] = None
#    for i_sb, subblock in block.items():
#        pages = set()
#        create_spec = (parameter_spec[block_name] is None)
#        if create_spec:
#            parameter_spec[block_name] = {}
#        for param_name, param_node in subblock.items():
#            pages = pages | { reg_node['page'] for reg_node in param_node.values() }
#            single_param = {
#                i_reg: {
#                    k: v
#                    for k, v in reg_node.items()
#                    if k != 'page'
#                }
#                for i_reg, reg_node in param_node.items()
#            }
#            if create_spec:
#                parameter_spec[block_name][param_name] = single_param
#            else:
#                if parameter_spec[block_name][param_name] != single_param:
#                    print(block_name, param_name, i_sb, *dict_diff(parameter_spec[block_name][param_name], single_param))
#                    raise Exception
#        print(block_name, i_sb, pages)
#    print(parameter_spec[block_name])
#
#
#for ch_like in ['cm', 'calib']:
#    ch_params = set(parameter_spec['ch'])
#    param_names = set(parameter_spec[ch_like])
#    if ch_params != param_names:
#        print(ch_like, 'different', 'missing', ch_params-param_names, 'extra', param_names-ch_params)
#        continue
#    # keys are the same
#    modified = {
#        k: (parameter_spec['ch'][k], parameter_spec[ch_like][k])
#        for k in ch_params
#        if parameter_spec['ch'][k] != parameter_spec[ch_like][k] 
#    }
#    if len(modified) > 0:
#        print(modified)
#    else:
#        print(ch_like, 'matches ch')
#
