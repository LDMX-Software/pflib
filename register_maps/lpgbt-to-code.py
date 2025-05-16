#!/usr/bin/python

import yaml
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('input', help='input yaml file')
parser.add_argument('output', type=str, help='output file')
args = parser.parse_args()

fout= open(args.output,"w")

with open(args.input) as f:
    reg_map = yaml.safe_load(f)

fout.write('static void populate_registers() {\n')
for (reg,fields) in reg_map.items():

    fout.write('  def_reg("%s",0x%03x);\n'%(reg,int(fields["address"],0)))
    if len(fields)>2:
        for (subreg,masks) in fields.items():
            if subreg=="address": continue
            fout.write('  def_reg("%s",0x%03x,0x%02x,%d);\n'%(subreg,int(fields["address"],0),int(masks['bit_mask']),int(masks['offset'])))
fout.write('}\n')
fout.close()
