# Parameter to Register Mappings

This directory contains the mappings and the scripts needed to generate
the C++ Look Up Tables (LUTs) from these mappings.
These are directly copied from [Arnaud Steen / swamp-cpp](https://gitlab.cern.ch/asteen/swamp-cpp)
on CERN's GitLab (which may not be accessible if you do not have a CERN account).

The page and parameter names are not necessarily aligned with the manual unfortunately.
Some have been shortened (for example `channel_0` is just `ch_0`) but there are
others that are present in the YAML mappings here and not in the manual and ones
that are present in the manual but not present here.

The SWAMP code, the code in this directory, and the manual use the words "page" and "sub-block"
to mean the same thing: a specific group of up to 32 registers.

I do not know why the SWAMP YAML mappings are written using the pair of bytes to specify the address
instead of the manual-documented page/register form; however, switching between them is just
some bit arithmetic and is implemented within [byte-pair-to-page-reg.py](byte-pair-to-page-reg.py)
before writing the C++ headers which use the page/register form to reduce duplication.
This script also can write out YAML of the page/register form for inspection by a human
(or to replace the byte-pair form if we desire to in the future).
