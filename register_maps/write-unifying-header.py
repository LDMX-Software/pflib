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
    '--output',
    type=Path,
    help='where to write header to'
)
args = parser.parse_args()

with open(args.output, 'w') as f:
    f.write('/* auto generated from scratch */\n\n')
    f.write('#pragma once\n\n')
    f.write('// include the register maps for each ROC type/version\n')
    for rt in args.roc_types:
        f.write('#include "register_maps/{rt}.h"\n'.format(rt=rt))
    for rt in args.econ_types:
        f.write('#include "register_maps/{rt}.h"\n'.format(rt=rt))
    f.write('\n// name the register maps so they can be retrieved by name\n')
    f.write('const std::map<std::string, std::pair<const PageLUT&, const ParameterLUT&>>\n')
    all_types = args.roc_types + args.econ_types
    f.write('REGISTER_MAP_BY_TYPE = {\n')
    f.write(',\n'.join(
        '  {"%s", {%s::PAGE_LUT, %s::PARAMETER_LUT}}'%(rt,rt,rt)
        for rt in all_types
    ))
    f.write('\n};\n')
