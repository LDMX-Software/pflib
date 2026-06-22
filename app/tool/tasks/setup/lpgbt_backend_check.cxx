#include "lpgbt_backend_check.h"

#include <bitset>

#include "pflib/OptoLink.h"
#include "pflib/lpGBT.h"

ENABLE_LOGGING();

// each output group {0..3} has 2 bits shifted by group*2
static const uint16_t ULDATASOURCE1 = 0x129;
static const uint16_t DPDATAPATTERN[4] = {0x131, 0x130, 0x12f, 0x12e};

enum UpLinkDataSourceCode : int {
  NORMAL_DATA = 0,
  PRBS7 = 1,
  BIN_CNTR_UP = 2,
  BIN_CNTR_DOWN = 3,
  CONST_PATTERN = 4,
  CONST_PATTERN_INV = 5,
  DL_DATA_LOOPBACK = 6,
};

class ConfigureUpLinkDataSource {
  pflib::lpGBT& lpgbt_;
  UpLinkDataSourceCode choice_{0};

 public:
  ConfigureUpLinkDataSource(pflib::lpGBT& l) : lpgbt_{l} {}
  void choose(UpLinkDataSourceCode choice) { choice_ = choice; }
  void configure(int ilink) {
    int grp = ilink;
    // on lpGBT mezzanine, links 3, 4, 5 are connected to eTx groups 4, 5, 6
    if (grp > 2) grp++;
    uint16_t datasource_config_reg = ULDATASOURCE1 + (grp / 2);
    uint8_t datasource = lpgbt_.read(datasource_config_reg);
    datasource |= (static_cast<int>(choice_) << ((grp % 2) * 3));
    lpgbt_.write(datasource_config_reg, datasource);
    printf("apply data source config 0x%02x on reg 0x%03x\n", datasource,
           datasource_config_reg);
  }
  void check(const std::vector<uint32_t> spy) {
    switch (choice_) {
      case UpLinkDataSourceCode::PRBS7: {
        std::bitset<64 * 32> data;
        for (int iword{0}; iword < 64; iword++) {
          for (int ibit{0}; ibit < 32; ibit++) {
            data[32 * iword + ibit] = ((spy[iword] >> (31 - ibit)) & 0x1);
          }
        }
        std::bitset<64 * 32> check = data;
        check = ((check >> 6) ^ (check >> 5));
        check <<= 12;
        auto diff = ((check >> 12) ^ (data >> 12));
        int correct_runlen{0};
        for (int ibit{0}; ibit < diff.size(); ibit++) {
          if (diff.test(ibit)) {
            // printf("%d bits before error\n");
            correct_runlen = 0;
          } else {
            correct_runlen++;
          }
        }
        printf("%u / %u bit errors\n", diff.count(), diff.size() - 12);
      } break;
      case UpLinkDataSourceCode::CONST_PATTERN: {
        uint32_t const_pattern{0};
        for (int i_byte{0}; i_byte < 4; i_byte++) {
          const_pattern |= (lpgbt_.read(DPDATAPATTERN[i_byte]) << (i_byte * 8));
        }
        printf("%u / %u words matched the const pattern 0x%08x\n",
               std::count(spy.begin(), spy.end(), const_pattern), spy.size(),
               const_pattern);
      } break;
      default:
        break;
    }
  }
};

void lpgbt_backend_check(Target* target) {
  bool daq = pftool::readline_bool("Check DAQ (Y) or TRG (N) lpGBT? ", true);
  pflib::lpGBT lpgbt{
      target->get_opto_link(daq ? "DAQ" : "TRG").lpgbt_transport()};

  int output_choice = pftool::readline_int(
      "Test Choice:\n  1. PRBS7\n  2. Binary Counter\n  3. Const Pattern\n", 3);
  if (output_choice < 1 or output_choice > 3) {
    pflib_log(error) << "unrecognized output choice " << output_choice;
    return;
  }

  ConfigureUpLinkDataSource conf{lpgbt};
  if (output_choice == 1) {
    conf.choose(UpLinkDataSourceCode::PRBS7);
  } else if (output_choice == 2) {
    bool up = pftool::readline_bool("Count up (y) or down (n)? ", true);
    if (up) {
      conf.choose(UpLinkDataSourceCode::BIN_CNTR_UP);
    } else {
      conf.choose(UpLinkDataSourceCode::BIN_CNTR_DOWN);
    }
  } else if (output_choice == 3) {
    uint32_t known_pattern = 0x12345678;
    known_pattern =
        pftool::readline_int("32b const pattern to use: ", known_pattern, true);
    printf("const pattern: 0x%08x\n", known_pattern);
    for (int i_byte{0}; i_byte < 4; i_byte++) {
      lpgbt.write(DPDATAPATTERN[i_byte],
                  ((known_pattern >> (i_byte * 8)) & 0xff));
    }
    bool invert = pftool::readline_bool("Invert the constant pattern? ", false);
    if (invert) {
      conf.choose(UpLinkDataSourceCode::CONST_PATTERN_INV);
    } else {
      conf.choose(UpLinkDataSourceCode::CONST_PATTERN);
    }
  }

  int link = pftool::readline_int("which link (-1 for all)? ", -1);
  if (link < 0) {
    for (int ilink{0}; ilink < 6; ilink++) {
      conf.configure(ilink);
    }
  } else if (link > 5) {
    pflib_log(error) << "invalid link index";
    return;
  } else {
    conf.configure(link);
  }

  pflib::Elinks& elinks = target->elinks();
  std::vector<std::vector<uint32_t>> spy(6);
  int ilink_offset{daq ? 0 : 6};
  printf("word :");
  for (int ilink{0}; ilink < 6; ilink++) {
    spy[ilink] = elinks.spy(ilink_offset + ilink);
    printf("   Link %d", ilink);
  }
  printf("\n");
  for (int iword{0}; iword < 64; iword++) {
    printf("%4d :", iword);
    for (int ilink{0}; ilink < 6; ilink++) {
      printf(" %08x", spy[ilink][iword]);
    }
    printf("\n");
  }

  if (link < 0) {
    for (int ilink{0}; ilink < 6; ilink++) {
      printf("Link %d:\n", ilink);
      conf.check(spy[ilink]);
    }
  } else {
    conf.check(spy[link]);
  }

  // reset data source to all zeros for all groups (normal operation)
  for (int i{0}; i < 4; i++) {
    lpgbt.write(ULDATASOURCE1 + i, 0x00);
  }
}
