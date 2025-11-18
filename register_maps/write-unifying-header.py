"""Write the header that includes all of the different LUT options and names them"""

import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument(
    '--roc_types',
    help='list of ROC types that were generated headers',
    nargs='+'
)
parser.add_argument(
    '--econ_types',
    help='list of ECON types that were generated headers',
    nargs='+'
)
parser.add_argument(
    '--output-header',
    type=Path,
    help='where to write header to'
)
parser.add_argument(
    '--output-src',
    type=Path,
    help='where source file should be written to'
)
args = parser.parse_args()

lut_of_luts_type = 'const std::unordered_map<std::string, std::pair<const PageLUT&, const ParameterLUT&>>'

with open(args.output_header, 'w') as f:
    f.write('/* auto generated from scratch */\n\n')
    f.write('#pragma once\n\n')
    f.write('#include "register_maps/register_maps_types.h"\n\n')
    f.write('namespace pflib::register_maps {\n\n')
    f.write(f'{lut_of_luts_type}&\n')
    f.write('get();\n')
    f.write('\n}\n')

with open(args.output_src, 'w') as f:
    f.write('/* auto generated from scratch */\n\n')
    f.write('#include "register_maps/register_maps.h"\n')
    f.write('namespace pflib::register_maps {\n\n')
    f.write('// include the register maps for each ROC type/version\n')
    for rt in args.roc_types:
        f.write('#include "register_maps/{rt}.h"\n'.format(rt=rt))
    for rt in args.econ_types:
        f.write('#include "register_maps/{rt}.h"\n'.format(rt=rt))
    f.write(f'{lut_of_luts_type}&\n')
    f.write('get() {\n')
    f.write('  // name the register maps so they can be retrieved by name\n')
    all_types = args.roc_types + args.econ_types
    f.write(f'  static {lut_of_luts_type}\n')
    f.write('  REGISTER_MAP_BY_TYPE = {\n')
    f.write(',\n'.join(
        '    {"%s", {%s::PAGE_LUT, %s::PARAMETER_LUT}}'%(rt,rt,rt)
        for rt in all_types
    ))
    f.write('\n  };\n')
    f.write('  return REGISTER_MAP_BY_TYPE;\n')
    f.write('}\n')
    f.write('\n}\n')
