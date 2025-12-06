#pragma once

#include "../pftool.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <vector>

inline std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ',')) {
        out.push_back(item);
    }
    return out;
}

inline bool file_exists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void get_lpgbt_temps(Target* tgt);

