# pftool

The main executable product of pflib is the `pftool`.
This executable is designed to offer low-level through high-level interactions
with the polarfire connected to HGC ROC(s).
This tool opens up a rudimentary terminal with various menus and submenus with commands
for configuring the chips and aquiring data.

This document is meant to give an overview of its design and provide some
manual documentation on its properties. We assume that `pftool` is already
successfully built.

## General Notes

#### Running
A successful open of the tool looks like
```
 Polarfire firmware :    1.33
  Active DAQ links: 0 1 

   ... main menu ...

 >
```
Notice that the firmware version is printed. 
This shows a successful connection to the polarfire.
The indices of the active DAQ links are also printed which
can help the user double-check that their `pftoolrc` was loaded successfully.

The listing of commands in your current menu is always printed
after you execute a command. I will omit these menu printouts in this document.
The commands are listed in all-caps, but the tool is case-insensitive,
so I will often list the commands in all lower-case.

#### pftoolrc Files
The `pftool` looks at the following locations for a RC file to load configuration options.
1. File at the environment variable `PFTOOLRC` 
2. File named `pftoolrc` in the current directory
3. File named `.pftoolrc` in your home directory

**Tip** A potentially helpful alias is to prepend `pftool` with the environment
variable so that `pftool` always loads the RC file in this repository.
```
alias pftool='PFTOOLRC=/full/path/to/pflib/pftoolrc pftool'
```
This specific alias assumes that `pftool` is installed to a directory in the `PATH`
environment variable.

#### uHal Config Files
uHal needs to load a set of XML files defining the IP-bus mapping.
You can support this by either running pftool from within the uhal directory
or by providing the full path to that directory using the `ipbus_map_path` parameter
in the RC file.

#### Exiting
You can leave the tool at anytime by pressing ctrl+C.

## Comments on Specific Commands
Below I list example printouts of specific commands along
with follow-up comments. The tool cannot currently handle
directly entering a command from a sub-menu. You will need
to enter the submenu before entring the command. For example,
`DAQ.STATUS` should be interpreted (in keystrokes) as `DAQ<Enter>STATUS`.

### daq.status
```
-----Front-end FIFO-----
 Header occupancy : 0  Maximum occupancy : 1
 Next event info: 0xd3825016 (BX=22, RREQ=37, OR=101)  RREQ=101
-----Per-ROCLINK processing-----
 Link  ID  EN ZS FL EM
 0000 0500 01 00 00 01 08397889
 0001 0501 01 00 00 01 08397889
 0002 0502 01 01 00 01 08397895
 0003 0503 01 01 00 01 08397895
 0004 0504 01 01 00 01 08397895
 0005 0505 01 01 00 01 08397895
 0006 0506 01 01 00 01 08397895
 0007 0507 01 01 00 01 08397895
-----Off-detector FIFO-----
 NOT-FULL 0000EMPTY Events ready: 000 Next event size: 101
```
At UMN, we only have one HGC ROC and therefore only two active links.
This means all of the links except for the first two are fully zero
suppressed (ZS == 1).

The ID number is defined during the execution of `daq.standard` and is `(<fpga-id> << 8) + <link>`.
This is the status for a "standard" setup. You can get back to the "standard" setup
by running the `daq` commands `hard_reset`, `standard`, `enable` (in that order).

### roc.hard\_reset
This sets the HGC ROC back to its power-on default state, including resetting all of its parameters.
If this command is issued, any ROC parameters that were loaded (via `roc.load` need to be re-loaded).

### daq.hard\_reset
This forcefully clears the event buffers as if a run wasn't started *and* resets any DAQ parameters.

### elinks.spy
This should show the "idle" pattern (ac cc cc cc repeated) when the link is behaving correctly.

### elinks.bitsplit and elinks.delay
Theses parameters help "align" the links so that they correctly show the "idle" pattern when you `elinks.spy`.

### daq.setup.l1aparams
These parameters should be stable for a given setup. **Across setups**, the readout length should be 40. For UMN, the delay should be 15.
