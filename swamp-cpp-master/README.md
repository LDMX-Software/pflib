# swamp-cpp

## pre-requisite

- gcc > 11
- redis:
    ```
    yum install redis
    ```
    - hiredis:
    ```
    git clone https://github.com/redis/hiredis.git
    cd hiredis
    make
    make install
    ```
        Library installed in `/usr/lib64` on x86_64 and in `/usr/local/lib` on aarch64. With the latter case, it was needed to add `/usr/local/lib` in LD_LIBRARY_PATH
    - redis-plus-plus
    ```
    git clone https://github.com/sewenew/redis-plus-plus.git
    cd redis-plus-plus
    mkdir build
    cd build
    cmake ..
    make
    make install
    cd ..
    ```
    - link redis libraries:
    ```
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/local/lib64
    ```
- boost, boost-devel
- uhal
- emp:
    - installed manually because could not find RPMs for almalinux 9
    - check : https://codimd.web.cern.ch/Xjn45d9XQMWDVjBkLd0WFw#Install-EMP
- spdlog:
    - installed manually on aarch64, otherwise some header files were missing:
    ```
    git clone https://github.com/gabime/spdlog.git
    cd spdlog && mkdir build && cd build
    cmake .. && make -j
    ```


## Installation
```
git clone https://gitlab.cern.ch/asteen/swamp-cpp.git
mkdir build
cd build
cmake ..
make -j
(sudo?) make install
```
swamp library will be installed in `/opt/swamp/lib`. Header files will be installed in `/opt/swamp/include`

## Running the executables:

### Generate template for all ASIC cache:
```
./build/generate_asic_cache_template
``` 

### Unit tests:
```
./build/chip
./build/transport
./build/lpgbt
./build/roc
./build/econ
./build/lpgbt_i2c
./build/lpgbt_gpio
```

Unit test `./build/sct` and `./build/emp` need to run in dedicated board (zcu or serenity)

### Command line arguments executables:

- All executables have a help option:
```
./build/${EXEC_NAME} -h
```
- Read/write to a LPGBT:
    ```
    ./build/lpgbt_control -h
    ```
    will return:
    ```
    lpgbt control options:
    -h [ --help ]                         Print help messages
    -F [ --hardwareConfigFile ] arg (=test/config/sct_test.yaml)
                                            hardware configuration file
    -l [ --lpgbtName ] arg (=daq_lpgbt)   name of the lpgbt you want to control
    -r [ --read_write ] arg (=0)          read write flag, 0:write , 1:read
    -L [ --logLevel ] arg (=2)            log level : 0-Trace; 1-Debug; 2-Info; 
                                            3-Warn; 4-Error; 5-Critical; 6-Off
    -f [ --configFile ] arg (=test/config/setup_daq_lpgbt.yaml)
                                            configuration file use to read/write 
                                            the lpgbt
    -c [ --compareFlag ] arg (=0)         flag to set in read mode if you want to
                                            compare the read content with the 
                                            initial configuration
    ```
    - write cfg from lpgbt (the help is/was wrong `-r 1` will perform a `write`):
    ```
    ./build/lpgbt_control -F test/config/sct_test.yaml -l lpgbt_daq -r 1 -f test/config/setup_daq_lpgbt.yaml
    ```
    - read cfg from lpgbt and compare to expectation:
    ```
    ./build/lpgbt_control -F test/config/sct_test.yaml -l lpgbt_daq -r 0 -f test/config/setup_daq_lpgbt.yaml -c 1
    ```
- Read/write to one or several HGCROCs (controlled by the same lpGBT):
    ```
    ./build/roc_control -h
    ```
    will return:
    ```
    roc control options:
        -h [ --help ]                         Print help messages
        -F [ --hardwareConfigFile ] arg (=test/config/sct_test.yaml)
                                                hardware configuration file
        -l [ --lpgbtName ] arg (=lpgbt_trg_w) name of the lpgbt you want to control
        -r [ --rocs ] arg                     names of the rocs you want to control
        -f [ --rocConfigFile ] arg            path to configuration file use to 
                                                read/write the roc
        -p [ --pname ] arg                    roc parammeter name to read or write, 
                                                expected format : block.subblock.paramn
                                                ame (if a rocConfigFile is given, pname
                                                will be ignored)
        -v [ --value ] arg                    value to write in roc parameter 
                                                (ignored if write flag==0 and if the 
                                                rocConfigFile is used)
        -w [ --read_write ] arg (=0)          read write flag, 1:write , 0:read
        -c [ --compareFlag ] arg (=0)         flag to set in read mode if you want to
                                                compare the read content with the 
                                                initial configuration
        -C [ --configureI2CMaster ] arg (=0)  flag to set you want to reset and 
                                                configure the I2C master before 
                                                configuring/reading the ROC
        --I2CMFreq arg (=3)                   Frequency of I2C master (only set when 
                                                configureI2CMaster option is set)
        -L [ --logLevel ] arg (=2)            log level : 0-Trace; 1-Debug; 2-Info; 
                                                3-Warn; 4-Error; 5-Critical; 6-Off
    ```
    - write cfg to HGCROCs:
    ```
    ./build/roc_control -F test/config/sct_test.yaml -l lpgbt_trg_w -r roc0_w0 roc1_w0 roc0_w2 -f test/config/init_roc.yaml -w 1
    ```
    - write single param to HGCROCs:
    ```
    ./build/roc_control -F test/config/sct_test.yaml -l lpgbt_trg_w -r roc0_w0 roc1_w0 roc0_w2 -pname Top.0.RunL -w 1 -v 1
    ```
    - read cfg of HGCROC and compare to expectation:
    ```
    ./build/roc_control -F test/config/sct_test.yaml -l lpgbt_trg_w -r roc0_w0 roc1_w0 roc0_w2 -f test/config/init_roc.yaml -w 0 -c 1
    ```
    - read single param of HGCROC and compare to expectation:
    ```
    ./build/roc_control -F test/config/sct_test.yaml -l lpgbt_trg_w -r roc0_w0 roc1_w0 roc0_w2 -pname Top.0.RunL -w 0 -c 1 -v 1
    ```