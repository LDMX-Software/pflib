#include "get_lpgbt_temps.h"

#include "pflib/OptoLink.h"

struct CalibConstants {
    double vref_slope;
    double vref_offset;
    double temp_slope;
    double temp_offset;
};

void save_results(const std::string& results_file,
                  const std::string& lpgbt_name,
		  const uint32_t chipid,
                  int optimal_vref_tune)
{
    printf("\n--- Step 4: Saving Calibration Results ---\n");

    YAML::Node results;

    // Load existing YAML file if present
    std::ifstream fin(results_file);
    if (fin.good()) {
        try {
            results = YAML::Load(fin);
        } catch (...) {
            printf("  > WARNING: Failed to parse existing YAML. Starting fresh.\n");
        }
    }
    fin.close();

    // Insert / update entry
    YAML::Node entry;
    entry["chipid"]    = chipid;
    entry["vref_tune"] = optimal_vref_tune;

    results[lpgbt_name] = entry;

    // Write YAML back
    std::ofstream fout(results_file);
    if (!fout.good()) {
        printf("  > ERROR: Could not write YAML file '%s'\n", results_file.c_str());
        return;
    }

    fout << results;
    fout.close();

    printf("  > Successfully wrote calibration data to '%s'\n", results_file.c_str());
    printf("\n---------------------------------------------------------\n");
    printf("  >>> Optimal VREFTUNE for %08X is: %d <<<\n",
           chipid, optimal_vref_tune);
    printf("---------------------------------------------------------\n");
}


static void get_calib_constants(Target* tgt) {

	pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};

	CalibConstants calib;
	const std::string& lpgbt_name = "DAQ";

	uint8_t REG_VREFTUNE = 0x01d;

	printf(" --- Finding CHIPID ---\n");
	uint32_t chipid = lpgbt_daq.read_efuse(0);
	printf(" CHIPID: %08X\n", chipid);
	
	char chipid_str[9];          // 8 hex chars + null terminator
	snprintf(chipid_str, sizeof(chipid_str), "%08X", chipid);
	std::string chipid_hex = chipid_str;

	printf(" --- Fetching calibration constants from .csv for %08X ---\n", chipid);
	std::string csv_filename = "lpgbt_calibration.csv";
	
	if (!file_exists(csv_filename)) {
        	printf("  > Local file '%s' not found...\n",
                csv_filename.c_str());
	} else {
		printf("  > Found local calibration file: '%s'\n", csv_filename.c_str());
	}

    	printf("  > Parsing CSV data...\n");

    	std::ifstream file(csv_filename);
    	if (!file.is_open()) {
        	printf("  > ERROR: Could not open CSV.\n");
        	printf("  > WARNING: LOADING HARDCODED VALUES FOR UVA DAQ LPGBT\n");
		
		calib.vref_slope  = -0.3108;
		calib.vref_offset = 117.1;
		calib.temp_slope  = 0.4546;
		calib.temp_offset = -206.1;
    	} else {

		std::string line;
		// Skip first three lines
		for (int i = 0; i < 3; i++) std::getline(file, line);

    		std::getline(file, line);
    		auto header = split_csv(line);


    		// Build name → index map
    		std::unordered_map<std::string, int> col;
    		for (int i = 0; i < header.size(); i++)
        		col[header[i]] = i;

    		// Search row by row for  CHIPID
    	
		bool found = false;
    		while (std::getline(file, line)) {
        		auto row = split_csv(line);
        		if (row[col["CHIPID"]] == chipid_hex) {
            			calib.vref_slope  = std::stod(row[col["VREF_SLOPE"]]);
            			calib.vref_offset = std::stod(row[col["VREF_OFFSET"]]);
            			calib.temp_slope  = std::stod(row[col["TEMPERATURE_UNCALVREF_SLOPE"]]);
            			calib.temp_offset = std::stod(row[col["TEMPERATURE_UNCALVREF_OFFSET"]]);
            			found = true;
            			break;
        		}
   		}
	}	
	
    	// if (!found) {
        // 	printf("  > ERROR: CHIPID '%s' not found in calibration DB.\n",
        //        	chipid.c_str());
        // 	return;
    	// }

    	printf("  > Success! Loaded constants for %08X\n", chipid);

	printf(" --- Performing Live VREFTUNE Calibration ---\n");
	printf("  > Taking uncalibrated reading with VREFTUNE = 0...\n");

	lpgbt_daq.write(REG_VREFTUNE, 0);
	uint8_t vreftune = lpgbt_daq.read(REG_VREFTUNE);
	printf("  > VREFTUNE REG: %d\n", vreftune);

	uint16_t adc_value = lpgbt_daq.adc_read(15, 15, 1);

	printf("  > ADC Value: %d\n", adc_value);

	float tj_user = adc_value * calib.temp_slope + calib.temp_offset;
	printf("  > Esitimated junction temperature (TJ_USER): %f degrees C\n", tj_user);

	int optimal_vref_tune = static_cast<int>(std::round(tj_user * calib.vref_slope + calib.vref_offset));
	printf("  > Optimal VREFTUNE: %d\n", optimal_vref_tune);
 
	save_results("calibration_results.yaml",
                lpgbt_name,
		chipid,
                optimal_vref_tune);
}


void get_lpgbt_temps(Target* tgt) {

	get_calib_constants(tgt);

}
