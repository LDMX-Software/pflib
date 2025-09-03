# HGC ROC YAML Settings
This is a collection of YAML settings that have been loaded onto the HGC ROCs and are generally considered "important" for some reason.
Below is a table attempting to describe the use case for the various files in this directory.

File | Description
---|---
top.yaml | Load necessary Top parameters for readout
globalana\_refvolt\_mastertdc.yaml | Other global parameters that can be loaded once for all channels
charge\_injection.yaml | Deactivate most channels and selectively activate a few channels for charge injection
disable-tot-toa.yaml | Deactivate TOA and TOT in all channels to suppress noise before thresholds are set
Lund\_hgcroc\_0\_settings\_20250723\_143207.yaml | Configuration file for the HGCROC in Lund after pedestal leveling