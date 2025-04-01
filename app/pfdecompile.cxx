/**
 * "decompiler" translating register values into
 * HGCROC named parameters and their valuesj
 *
 * Only compiled and installed if yaml-cpp is found by CMake.
 */

#include <iostream>
#include <iomanip>
#include <fstream>

#include <yaml-cpp/yaml.h>

#include "pflib/Exception.h"
#include "pflib/Logging.h"
#include "pflib/Compile.h"

static void usage() {
  std::cout <<
    "\n"
    " USAGE:\n"
    "  pfdecompile [options] register_values\n"
    "\n"
    " OPTIONS:\n"
    "  -h,--help     : Print this help and exit\n"
    "  --no-careful  : Don't print warnings and use 0 for missing registers\n"
    "  --careful     : Print warnings and skip parameters with missing registers (default)\n"
    "  -r,--roc      : Define the ROC type_version that should be used for compilation\n"
    "                  By default, we use the sipm_rocv3b register mapping.\n"
    "  --output, -o  : Define the output file.\n"
    "                  By default, the output file is the file with register values with extension\n"
    "                  changed to 'yaml'\n"
    << std::endl;
}

/**
 * Modern RAII-styled CSV extractor taken from
 * https://stackoverflow.com/a/1120224/17617632
 * this allows us to **discard white space within the cells**
 * making the CSV more human readable.
 */
static std::vector<int> getNextLineAndExtractValues(std::istream& ss) {
  std::vector<int> result;

  std::string line, cell;
  std::getline(ss, line);

  if (line.empty() or line[0] == '#') {
    // empty line or comment, return empty container
    return result;
  }

  std::stringstream line_stream(line);
  while (std::getline(line_stream, cell, ',')) {
    /**
     * std stoi has a auto-detect base feature
     * https://en.cppreference.com/w/cpp/string/basic_string/stol
     * which we can enable by setting the pre-defined base to 0
     * (the third parameter) - this auto-detect base feature
     * can handle hexidecial (prefix == 0x or 0X), octal (prefix == 0),
     * and decimal (no prefix).
     *
     * The second parameter is an address to put the number of characters processed,
     * which I disregard at this time.
     *
     * Do we allow empty cells?
     */
    result.push_back(cell.empty() ? 0 : std::stoi(cell,nullptr,0));
  }
  // checks for a trailing comma with no data after it
  if (!line_stream and cell.empty()) {
    // trailing comma, put in one more 0
    result.push_back(0);
  }

  return result;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    usage();
    return 1;
  }

  pflib::logging::open(isatty(STDOUT_FILENO));
  auto the_log_{pflib::logging::get("pfdecompile")};

  bool careful{true};
  std::string input_filename;
  std::string output_filename;
  std::string roc_type_version;
  for (int i_arg{1}; i_arg < argc; i_arg++) {
    std::string arg{argv[i_arg]};
    if (arg[0] == '-') {
      // option
      if (arg == "--help" or arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--roc" or arg == "-r") {
        if (i_arg+1 == argc or argv[i_arg+1][0] == '-') {
          pflib_log(fatal) << "The " << arg << " parameter requires are argument after it.";
          return 1;
        }
        i_arg++;
        roc_type_version = argv[i_arg];
      } else if (arg == "--output" or arg == "-o") {
        if (i_arg+1 == argc or argv[i_arg+1][0] == '-') {
          pflib_log(fatal) << "The " << arg << " parameter requires are argument after it.";
          return 1;
        }
        i_arg++;
        output_filename = argv[i_arg];
      } else if (arg == "--no-careful") {
        careful = false;
      } else if (arg == "--careful") {
        careful = true;
      } else {
        pflib_log(fatal) <<  arg << " not a recognized argument.";
        return 1;
      }
    } else {
      // positional ==> settings file
      if (not input_filename.empty()) {
        pflib_log(fatal) << "We can only decompile one file with register values.";
        return 1;
      }
      input_filename = arg;
    }
  }

  if (input_filename.empty()) {
    pflib_log(fatal) << "We need one file to decompile.";
    return 1;
  }

  if (output_filename.empty()) {
    // strip extension
    output_filename = input_filename.substr(0,input_filename.find_last_of('.'));
    // add yaml extension
    output_filename += ".yaml";
  }

  std::ifstream inf{input_filename};
  if (not inf.is_open()) {
    pflib_log(fatal) << "Unable to open input file " << input_filename;
    return 2;
  }

  // try to open file before decompilation to make sure we have access
  std::ofstream of{output_filename};
  if (not of.is_open()) {
    pflib_log(fatal) << "Unable to open output file " << output_filename;
    return 3;
  }

  std::map<int,std::map<int,uint8_t>> settings;
  while (inf) {
    auto cells = getNextLineAndExtractValues(inf);
    if (cells.empty()) continue;
    if (cells.size() != 3) {
      pflib_log(warn) << "Skipping row with exactly three columns.";
      continue;
    }
    settings[cells.at(0)][cells.at(1)] = cells.at(2);
  }

  std::map<std::string,std::map<std::string,int>> parameters;
  try {
    // compilation checks parameter/page names
    parameters = pflib::Compiler::get(roc_type_version).decompile(settings,careful);
  } catch (const pflib::Exception& e) {
    pflib_log(fatal) <<  "[" << e.name() << "] " << e.message();
    return -1;
  }

  YAML::Emitter out;
  out << YAML::Comment("This YAML settings file was generated by pfdecompile");
  out << YAML::BeginMap;
  for (const auto& page : parameters) {
    out << YAML::Key << page.first;
    out << YAML::Value << YAML::BeginMap;
    for (const auto& param : page.second) {
      out << YAML::Key << param.first << YAML::Value << param.second;
    }
    out << YAML::EndMap;
  }
  out << YAML::EndMap;

  of << out.c_str() << std::endl;

  return 0;
}
