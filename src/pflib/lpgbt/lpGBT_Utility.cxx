#include "pflib/lpgbt/lpGBT_Utility.h"

#include <stdio.h>
#include <string.h>

#include "pflib/lpgbt/lpGBT_Registers.h"
#include "pflib/utility/str_to_int.h"

namespace pflib {
namespace lpgbt {

typedef std::vector<std::pair<std::string, uint8_t> > CSVDecodedLines;

static CSVDecodedLines decode_file(const std::string& fname) {
  CSVDecodedLines lines;
  char buffer[1024];

  FILE* f = fopen(fname.c_str(), "r");
  if (f == 0) {
    snprintf(buffer, 1024, "Unable to open CSV file '%s'", fname.c_str());
    PFEXCEPTION_RAISE("FileOpenException", buffer);
  }
  const char* delimiters = ",;\t";
  while (!feof(f)) {
    buffer[0] = 0;
    fgets(buffer, 1000, f);

    // truncate after any hash character
    char* hashpoint = strchr(buffer, '#');
    if (hashpoint) *hashpoint = 0;

    std::string reg;
    uint8_t val;
    char* token;
    char* ptr = buffer;
    for (int ifield = 0; (token = strsep(&ptr, delimiters)); ifield++) {
      std::string str;
      for (int i = 0; token[i] != 0; i++)
        if (!isspace(token[i])) str += token[i];

      if (ifield == 0)
        reg = str;
      else if (ifield == 1) {
        try {
          val = utility::str_to_int(str);
        } catch (std::exception& e) {
          char msg[100];
          snprintf(msg, 100, "Unable to decode string '%s'", str.c_str());
          PFEXCEPTION_RAISE("DecodeFailure", msg);
        }
        lines.push_back(std::make_pair(reg, val));
      }
    }
  }
  fclose(f);
  return lines;
}

lpGBT::RegisterValueVector loadRegisterCSV(const std::string& filename) {
  char mesg[120];
  CSVDecodedLines lines = decode_file(filename);
  lpGBT::RegisterValueVector retval;
  for (auto line : lines) {
    // see if this is a valid register...
    uint16_t addr;
    if (line.first.find("0x") == 0 || line.first.find("0b") == 0 ||
        line.first.find_first_not_of("0123456789") == std::string::npos) {
      addr = utility::str_to_int(line.first);
    } else {
      uint8_t mask, offset;
      if (!findRegister(line.first, addr, mask, offset)) {
        snprintf(mesg, 120, "Unknown register '%s'", line.first.c_str());
        PFEXCEPTION_RAISE("UnknownRegisterException", mesg);
      }
      if (mask != 0xFF) {
        snprintf(
            mesg, 120,
            "Invalid register for loadRegisterCSV, must be full register '%s'",
            line.first.c_str());
        PFEXCEPTION_RAISE("InvalidRegisterException", mesg);
      }
    }
    retval.push_back(std::make_pair(addr, line.second));
  }
  return retval;
}

void applylpGBTCSV(const std::string& filename, lpGBT& lpgbt) {
  char mesg[120];
  CSVDecodedLines lines = decode_file(filename);
  lpGBT::RegisterValueVector retval;
  for (auto line : lines) {
    // see if this is a valid register...
    uint16_t addr;
    uint8_t mask = 0xff, offset = 0;
    if (line.first.find("0x") == 0 || line.first.find("0b") == 0 ||
        line.first.find_first_not_of("0123456789") == std::string::npos) {
      addr = utility::str_to_int(line.first);
    } else {
      if (!findRegister(line.first, addr, mask, offset)) {
        snprintf(mesg, 120, "Unknown register '%s'", line.first.c_str());
        PFEXCEPTION_RAISE("UnknownRegisterException", mesg);
      }
    }
    if (mask == 0xff) {
      lpgbt.write(addr, line.second);
    } else {
      uint8_t val = lpgbt.read(addr);
      uint8_t antimask = 0xFF ^ mask;
      val = (val & antimask) | ((line.second << offset) & mask);
      lpgbt.write(addr, val);
    }
  }
}

}  // namespace lpgbt
}  // namespace pflib
