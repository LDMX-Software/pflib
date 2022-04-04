# Configure the Detector
The executable and related files in this directory are focused
on applying a configuration to the entire detector across multiple
polarfires.

The executable takes a single YAML file with all of the configuration
parameters that you wish to apply to the detector. The recursive nature
of YAML allows us to define a meta-format for this configuration file which
is documented below.

## YAML Format
The YAML format is a format that depends on whitespace, so be careful.
Comments are allowed and begin with the `#` character.
Our meta-format on top of YAML has a nested-dictionary structure allowing
for any level of specificity.

In general, each polarfire is an entry in a YAML dictionary where
the key is the hostname to connect to that polarfire (e.g. `cob1-dpm1`)
and the value is another YAML dictionary with setting name and setting node
pairs. The `<setting_node>` is indicating that the value of each setting can
take on an arbitrarily deep YAML tree (biggest current one is the `hgcrocs` setting below).

```yaml
polarfire-hostname:
  setting_name : 
    <setting_node>
```

### `inherit` keyword
This format also has a special keyword *inherit* if a top-level key takes on this
value, then that setting will be applied to all other polarfires in the config.
```yaml
inherit:
  seting_name : value_will_be_inherited
pf1:
  setting_name : override_value
pf2:
  # will have setting_name = value_will_be_inherited
```

**For the `hgcrocs` setting specifically**, the `inherit` keyword can also be
used in order to apply HGC ROC parameters to all ROCs listed.
```yaml
polarfire-hostname:
  hgcrocs:
    inherit:
      top:
        phase: 9
    0:
      top:
        phase 8
    1:
      # will have top.phase = 9
``` 
The two available uses of `inherit` can be combined to apply HGC ROC parameters
across all ROCs and all polarfires listed.

```yaml
inherit:
  hgcrocs:
    inherit:
      top:
        phase: 9
pf1:
  hgcrocs:
    0:
      # will have top.phase = 9
pf2:
  hgcrocs:
    1:
      # will have top.phase = 9
    2:
      # will have top.phase = 9
```

**Note** `inherit` detects which polarfires and which hgcrocs to apply the parameters
to by crawling the YAML file. This means you _need_ to list the polarfire/hgcroc even
if it doesn't specify any individual parameters (this is what I do in the examples above).
Specifically in reference to the example above, _even if_ there is a roc with 
index `0` at `pf2`, that roc will not recieve the `top.phase = 9` setting.

### HGC ROC Parameters
The HGC ROC parameters compiled into registers and loaded through pftool
under the `ROC.LOAD` settings have the same structure here except moved over
three indentations in order to accomodate the selection of a specific Polarfire
and a specific ROC on that polarfire. They are housed under the `hgcrocs` setting.
```yaml
  hgcrocs:
    roc_index :
      page_name :
        param_name : param_val
```
Alternatively, the value can also be a path to another YAML file to be used.
```yaml
  hgcrocs:
    roc_index: '/full/path/to/hgcroc_parameters.yaml'
```

## Other Polarfire Settings
As mentioned above, the YAML format is extremely flexible and allows for arbitrary 
YAML trees to be parsed by classes "applying" those settings. Other classes "applying"
settings can be implemented with the only restriction being that it can be executed
via a pointer to `PolarfireTarget` (same limitation as pftool commands).

### calib\_offset
This setting is a single integer that is the same as `FC.CALIB` in pftool.
```yaml
  calib_offset: 16
```

### sipm\_bias
This setting takes a YAML map with two keys. 
`value` sets the SiPM bias ADC value and `rocs` lists the roc indices to apply this to.
This is similar to `ROC.BIAS` in pftool.
```yaml
  sipm_bias:
    value: 3784
    rocs: [0,2,3]
```

### Implementing a New Setting
Making a setting available to be applied by this executable is done with a library/factory model.
Each setting to be applied has multiple requirements.
1. Inherit from `pflib::detail::PolarfireSetting`
2. Implement the `import` function which _updates_ the settings.
   I emphasize update here because with the `inherit` keyword, 
   settings should be designed to be specialized by separate polarfires.
3. Implement the `execute` function which takes a PolarfireTarget and applies those settings.
4. Implement `stream` which prints out the _value_ to the passed stream in a human
   readable way.
5. Register the setting in the anonymous namespace.

The examples given in [DetectorConfiguration.cxx](DetectorConfiguration.cxx) is a good place to start.
