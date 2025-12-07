#include "get_lpgbt_temps.h"

#include "pflib/OptoLink.h"

struct CalibConstants {
    int vref_tune;
    double tj_user;
    double vref_slope;
    double vref_offset;
    double TEMPERATURE_UNCALVREF_SLOPE;
    double TEMPERATURE_UNCALVREF_OFFSET;
    double TEMPERATURE_SLOPE;
    double TEMPERATURE_OFFSET;
    double ADC_X2_SLOPE;
    double ADC_X2_SLOPE_TEMP;
    double ADC_X2_OFFSET;
    double ADC_X2_OFFSET_TEMP;
};

uint8_t REG_VREFTUNE = 0x01d;
uint8_t REG_CURDACVALUE = 0x06c;

void save_results(const std::string& results_file,
                  const std::string& lpgbt_name,
		  const uint32_t chipid,
                  int optimal_vref_tune,
		  const CalibConstants& calib)
{
    printf(" --- Saving Calibration Results ---\n");

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

    char buf[9];
    snprintf(buf, sizeof(buf), "%08X", chipid);
    std::string chipid_hex = buf;
    // Insert / update entry
    YAML::Node entry;
    entry["chipid"]    = chipid_hex;
    entry["vref_tune"] = optimal_vref_tune;

    YAML::Node cal;
    cal["vref_slope"]  = calib.vref_slope;
    cal["vref_offset"] = calib.vref_offset;
    cal["TEMPERATURE_SLOPE"]  = calib.TEMPERATURE_SLOPE;
    cal["TEMPERATURE_OFFSET"] = calib.TEMPERATURE_OFFSET;
    cal["TEMPERATURE_UNCALVREF_SLOPE"]  = calib.TEMPERATURE_UNCALVREF_SLOPE;
    cal["TEMPERATURE_UNCALVREF_OFFSET"] = calib.TEMPERATURE_UNCALVREF_OFFSET;
    cal["ADC_X2_SLOPE"] = calib.ADC_X2_SLOPE;
    cal["ADC_X2_SLOPE_TEMP"] = calib.ADC_X2_SLOPE_TEMP;
    cal["ADC_X2_OFFSET"] = calib.ADC_X2_OFFSET;
    cal["ADC_X2_OFFSET_TEMP"] = calib.ADC_X2_OFFSET_TEMP;

    entry["calib_constants"] = cal;

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

CalibConstants calculate_tj_user(const std::string& results_file,
                       const std::string& lpgbt_name)
{
    
    CalibConstants c;
    YAML::Node results = YAML::LoadFile(results_file);

    if (!results[lpgbt_name]) {
        std::cerr << "ERROR: No entry found for '" << lpgbt_name
                  << "' in results file.\n";
        return c;
    }

    YAML::Node entry = results[lpgbt_name];

    // Extract vref_tune
    if (!entry["vref_tune"]) {
        std::cerr << "ERROR: vref_tune missing in YAML entry.\n";
        return c;
    }
    int vref_tune = entry["vref_tune"].as<int>();

    // Extract calibration constants
    if (!entry["calib_constants"]) {
        std::cerr << "ERROR: calib_constants missing in YAML entry.\n";
        return c;
    }

    YAML::Node cal = entry["calib_constants"];

    c.vref_slope  = cal["vref_slope"].as<double>();
    c.vref_offset = cal["vref_offset"].as<double>();
    c.TEMPERATURE_SLOPE  = cal["TEMPERATURE_SLOPE"].as<double>();    
    c.TEMPERATURE_OFFSET = cal["TEMPERATURE_OFFSET"].as<double>();
    c.TEMPERATURE_UNCALVREF_OFFSET = cal["TEMPERATURE_UNCALVREF_OFFSET"].as<double>();
    c.TEMPERATURE_UNCALVREF_SLOPE  = cal["TEMPERATURE_UNCALVREF_SLOPE"].as<double>();
    c.ADC_X2_SLOPE = cal["ADC_X2_SLOPE"].as<double>();
    c.ADC_X2_SLOPE_TEMP = cal["ADC_X2_SLOPE_TEMP"].as<double>();
    c.ADC_X2_OFFSET = cal["ADC_X2_OFFSET"].as<double>();
    c.ADC_X2_OFFSET_TEMP = cal["ADC_X2_OFFSET_TEMP"].as<double>();

    c.vref_tune = entry["vref_tune"].as<int>();
    // Compute TJ_USER
    c.tj_user = (vref_tune - c.vref_offset) / c.vref_slope;

    printf("  > Inferred Junction Temperature (TJ_USER) from vref_tune: %f degrees C\n",
           c.tj_user);
    return c;
}

static void get_calib_constants(Target* tgt) {

	pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};

	CalibConstants calib;
	const std::string& lpgbt_name = "DAQ";

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
		calib.TEMPERATURE_SLOPE  = 415;
		calib.TEMPERATURE_OFFSET = -184;
		calib.TEMPERATURE_UNCALVREF_SLOPE  = 0.455;
		calib.TEMPERATURE_UNCALVREF_OFFSET = -206;
		calib.ADC_X2_SLOPE = 0.00104;
	        calib.ADC_X2_SLOPE_TEMP = -0.00000000309;
		calib.ADC_X2_OFFSET = -0.0302;
		calib.ADC_X2_OFFSET_TEMP = -0.00000862;
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
            			calib.TEMPERATURE_SLOPE  = std::stod(row[col["TEMPERATURE_SLOPE"]]);
            			calib.TEMPERATURE_OFFSET = std::stod(row[col["TEMPERATURE_OFFSET"]]);
            			calib.TEMPERATURE_UNCALVREF_SLOPE  = std::stod(row[col["TEMPERATURE_UNCALVREF_SLOPE"]]);
            			calib.TEMPERATURE_UNCALVREF_OFFSET = std::stod(row[col["TEMPERATURE_UNCALVREF_OFFSET"]]);
        			calib.ADC_X2_SLOPE       = std::stod(row[col["ADC_X2_SLOPE"]]);
        			calib.ADC_X2_SLOPE_TEMP  = std::stod(row[col["ADC_X2_SLOPE_TEMP"]]);
        			calib.ADC_X2_OFFSET      = std::stod(row[col["ADC_X2_OFFSET"]]);
        			calib.ADC_X2_OFFSET_TEMP = std::stod(row[col["ADC_X2_OFFSET_TEMP"]]);
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

	uint16_t adc_value = lpgbt_daq.adc_read(14, 14, 16);

	printf("  > ADC Value: %d\n", adc_value);

	float tj_user = adc_value * calib.TEMPERATURE_UNCALVREF_SLOPE + calib.TEMPERATURE_UNCALVREF_OFFSET;
	printf("  > Esitimated junction temperature (TJ_USER): %f degrees C\n", tj_user);

	int optimal_vref_tune = static_cast<int>(std::round(tj_user * calib.vref_slope + calib.vref_offset));
	printf("  > Optimal VREFTUNE: %d\n", optimal_vref_tune);
 
	save_results("calibration_results.yaml",
                lpgbt_name,
		chipid,
                optimal_vref_tune,
		calib);
}

static uint16_t read_adc_at_current(Target* tgt, const std::string& sensor_name, int curdacvalue, const CalibConstants& constants) {
	pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};
	
	lpgbt_daq.write(REG_CURDACVALUE, curdacvalue);
	lpgbt_daq.write(REG_VREFTUNE, constants.vref_tune);
	uint8_t vreftune = lpgbt_daq.read(REG_VREFTUNE);
	printf("  > VREFTUNE REG: %d\n", vreftune);

	uint16_t adc_value = lpgbt_daq.adc_read(14, 14, 16);

	printf("  > Raw ADC Value: %d\n", adc_value);
	return adc_value;
}

static void read_internal_temp(Target* tgt) {

	CalibConstants constants = calculate_tj_user("calibration_results.yaml", "DAQ");

	uint16_t adc_value = read_adc_at_current(tgt, "DAQ", 0, constants);

    	double adc_slope       = constants.ADC_X2_SLOPE;
    	double adc_slope_temp  = constants.ADC_X2_SLOPE_TEMP;
    	double adc_offset      = constants.ADC_X2_OFFSET;
        double adc_offset_temp = constants.ADC_X2_OFFSET_TEMP;
	double tj_user         = constants.tj_user;
	double temp_slope      = constants.TEMPERATURE_SLOPE;
	double temp_offset     = constants.TEMPERATURE_OFFSET;

	double vadc_v = adc_value * (adc_slope + tj_user * adc_slope_temp) + adc_offset + tj_user * adc_offset_temp;

	printf("  > Calibrated Voltage: %.4f V\n", vadc_v);
	double temperature_c = (vadc_v * constants.TEMPERATURE_SLOPE) + constants.TEMPERATURE_OFFSET;
	printf("  > Internal Temperature: %.2f degrees C\n", temperature_c);
}

void get_lpgbt_temps(Target* tgt) {


    const std::string yaml_file = "calibration_results.yaml";

    if (file_exists(yaml_file)) {
        printf("  > Found existing calibration file '%s'. Skipping calibration step.\n",
               yaml_file.c_str());

        read_internal_temp(tgt);
    } else {
        printf("  > No calibration file found. Running full calibration...\n");

        get_calib_constants(tgt);
        read_internal_temp(tgt);
    }
	//get_calib_constants(tgt);
	//read_internal_temp(tgt);

}
