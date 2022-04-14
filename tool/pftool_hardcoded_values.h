#ifndef PFTOOL_HARDCODED_VALUES_H
#define PFTOOL_HARDCODED_VALUES_H

#include <vector>

struct calibrun_hardcoded_values {
    std::vector<int> led_dac_values{1550, 1570, 1590, 1610, 1630, 1650};

    int num_led_events = 10000;
    int led_calib_length = 2;
    int led_calib_offset = 19;
    int led_l1offset = 8;
    int charge_l1offset = 8;
    int calib_length = 2;
    int calib_offset = 14;
    int SiPM_bias = 3784;
    int lowrange_dac_min = 0;
    int lowrange_dac_max = 2047;
    int highrange_fine_dac_min = 0;
    int highrange_fine_dac_max = 300;
    int highrange_coarse_dac_min = 300;
    int highrange_coarse_dac_max = 2047;
    int highrange_dac_max = 2047;
    int coarse_steps = 10;
    int fine_steps = 20;
    int events_per_step = 3;

    // Number of samples - 1
    int num_extra_samples = 7;

    std::string l1offset_page = "DIGITAL_HALF_";
    std::string l1offset_parameter = "L1OFFSET";

    std::string intctest_page = "REFERENCE_VOLTAGE_";
    std::string intctest_parameter = "INTCTEST";



};

#endif /* PFTOOL_HARDCODED_VALUES_H */
