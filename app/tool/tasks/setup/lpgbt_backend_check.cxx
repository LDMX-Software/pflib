#include "lpgbt_backend_check.h"

#include <bitset>
#include <optional>

#include "pflib/OptoLink.h"
#include "pflib/lpGBT.h"
#include "pflib/packing/Hex.h"
using pflib::packing::hex;

ENABLE_LOGGING();

static const uint16_t DPDATAPATTERN[4] = {0x131, 0x130, 0x12f, 0x12e};

class InjectTestPattern {
 protected:
  pflib::lpGBT& lpgbt_;
  void write_DPDataPattern(uint32_t val) {
    for (int i_byte{0}; i_byte < 4; i_byte++) {
      lpgbt_.write(DPDATAPATTERN[i_byte],
                  ((val >> (i_byte * 8)) & 0xff));
    }
  }
 public:
  InjectTestPattern(pflib::lpGBT& lpgbt) : lpgbt_{lpgbt} {}
  virtual ~InjectTestPattern() = default;
  virtual void check(const std::vector<uint32_t>& spy) = 0;
};

class InjectIntoSerializer : public InjectTestPattern {
 public:
  /**
   * These are the options to replace the serializer
   * data source.
   *
   * Table 14.1
   */
  enum Source : int {
    DATA = 0,
    PRBS7 = 1,
    PRBS15 = 2,
    PRBS23 = 3,
    PRBS31 = 4,
    CLK5G12 = 5,
    CLK2G56 = 6,
    CLK1G28 = 7,
    CLK40M = 8,
    DL_FRAME_10G24 = 9,
    DL_FRAME_5G12 = 10,
    DL_FRAME_2G56 = 11,
    CONST_PATTERN = 12
  };
  
  // ULSerTextPattern is lowset 4 bits of register 0x128
  static const uint16_t ULSERTESTPATTERN_REG = 0x128;

  InjectIntoSerializer(pflib::lpGBT& lpgbt, Source s, std::optional<uint32_t> known_pattern = {})
  : InjectTestPattern{lpgbt}, choice_{s}, known_pattern_{known_pattern} {
    // technically, sets ULECDataSource to 0 (EPORTRX_DATA)
    // and ULSerTestPattern to our choice
    lpgbt_.write(ULSERTESTPATTERN_REG, static_cast<int>(choice_));
    if (known_pattern_) {
      write_DPDataPattern(known_pattern_.value());
    }
  }

  void check(const std::vector<uint32_t>& spy) final {
    switch(choice_) {
      default:
        break;
    }
  }

  ~InjectIntoSerializer() {
    // technically, resets both ULSerTestPattern and ULECDataSource
    lpgbt_.write(ULSERTESTPATTERN_REG, 0x00);
    if (known_pattern_.has_value()) {
      write_DPDataPattern(0x00000000);
    }
  }

 private:
  Source choice_{Source::DATA};
  std::optional<uint32_t> known_pattern_;
};

class InjectUpLinkDataSource : public InjectTestPattern {
 public:
  // each output group {0..3} has 2 bits shifted by group*2
  static const uint16_t ULDATASOURCE1 = 0x129;

  /**
   * These are the "Up Link Pattern Gen" options
   * to replace the data from a given eRx group.
   *
   * Table 14.2
   */
  enum Source : int {
    NORMAL_DATA = 0,
    PRBS7 = 1,
    BIN_CNTR_UP = 2,
    BIN_CNTR_DOWN = 3,
    CONST_PATTERN = 4,
    CONST_PATTERN_INV = 5,
    DL_DATA_LOOPBACK = 6,
  };

  InjectUpLinkDataSource(
    pflib::lpGBT& l,
    Source s,
    int ilink,
    std::optional<uint32_t> known_pattern = {}
  ) : InjectTestPattern{l}, choice_{s}, ilink_{ilink}, known_pattern_{known_pattern} {
    if (ilink < 0) {
      for (int i{0}; i < 6; i++) configure(i);
    } else if (ilink < 6) {
      configure(ilink);
    } else {
      PFEXCEPTION_RAISE("BadILink", "ilink is greater than 6 which is not allowed");
    }
    if (known_pattern_.has_value()) {
      write_DPDataPattern(known_pattern_.value());
    }
  }
  ~InjectUpLinkDataSource() {
    // reset data source to all zeros for all groups (normal operation)
    for (int i{0}; i < 4; i++) {
      lpgbt_.write(ULDATASOURCE1 + i, 0x00);
    }
    if (known_pattern_.has_value()) {
      write_DPDataPattern(0x00000000);
    }
  }
  void check(const std::vector<uint32_t>& spy) final {
    switch (choice_) {
      case Source::PRBS7: {
        std::bitset<64 * 32> data;
        for (int iword{0}; iword < 64; iword++) {
          for (int ibit{0}; ibit < 32; ibit++) {
            data[32 * iword + ibit] = ((spy[iword] >> (31 - ibit)) & 0x1);
          }
        }
        if (data.none()) {
          printf("ERROR: all collected bits are zero\n");
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
      case Source::CONST_PATTERN: {
        int bad_count{0};
        int zero_to_one{0}, one_to_zero{0};
        std::bitset<32> k{known_pattern_.value()};
        for (uint32_t word : spy) {
          if (word != known_pattern_.value()) {
            bad_count++;
          }
          std::bitset<32> w{word};
          for (std::size_t i_bit{0}; i_bit < w.size(); i_bit++) {
            // print same order as pattern that was typed in (MSB -> LSB)
            std::size_t i = w.size() - i_bit - 1;
            if (w[i] == 0 and k[i] == 0) {
              std::cout << "0";
            } else if (w[i] == 1 and k[i] == 1) {
              std::cout << "1";
            } else if (w[i] == 0 and k[i] == 1) {
              std::cout << "-";
              one_to_zero++;
            } else if (w[i] == 1 and k[i] == 0) {
              std::cout << "+";
              zero_to_one++;
            }
          }
          std::cout << std::endl;
        }
        printf("%u / %u words did NOT match the const pattern 0x%08x\n",
               bad_count, spy.size(), known_pattern_.value());
        printf("%u 0->1 errors and %u 1->0 errors out of %u bits\n",
                zero_to_one, one_to_zero, 32*spy.size());
      } break;
      default:
        break;
    }
  }
 private:
  int ilink_;
  Source choice_{Source::NORMAL_DATA};
  std::optional<uint32_t> known_pattern_;
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
};

void lpgbt_backend_check(Target* target) {
  bool daq = pftool::readline_bool("Check DAQ (Y) or TRG (N) lpGBT? ", true);
  pflib::lpGBT lpgbt{
      target->get_opto_link(daq ? "DAQ" : "TRG").lpgbt_transport()};

  int link = -1;
  std::unique_ptr<InjectTestPattern> injector;
  if (pftool::readline_bool("Inject test pattern into serializer (y) or eRx groups (n)? ", true)) {
    int output_choice = pftool::readline_int(
      "Test Choice:\n  1. PRBS7\n  2. CLK40M\n  3. Const Pattern\n", 3
    );
    if (output_choice < 1 or output_choice > 3) {
      pflib_log(error) << "unrecognized output choice" << output_choice;
      return;
    }
    if (output_choice == 1) {
      injector = std::make_unique<InjectIntoSerializer>(
        lpgbt,
        InjectIntoSerializer::Source::PRBS7
      );
    } else if (output_choice == 2) {
      injector = std::make_unique<InjectIntoSerializer>(
        lpgbt,
        InjectIntoSerializer::Source::CLK40M
      );
    } else if (output_choice == 3) {
      uint32_t known_pattern = 0x12345678;
      known_pattern =
        pftool::readline_int("32b const pattern to use: ", known_pattern, true);
      injector = std::make_unique<InjectIntoSerializer>(
        lpgbt,
        InjectIntoSerializer::Source::CONST_PATTERN,
        known_pattern
      );
    }
  } else {
    link = pftool::readline_int("which link (-1 for all)? ", -1);
    if (link > 5) {
      pflib_log(error) << "invalid link index";
      return;
    }

    int output_choice = pftool::readline_int(
       "Test Choice:\n  1. PRBS7\n  2. Binary Counter\n  3. Const Pattern\n", 3);
    if (output_choice < 1 or output_choice > 3) {
      pflib_log(error) << "unrecognized output choice " << output_choice;
      return;
    }
    if (output_choice == 1) {
      injector = std::make_unique<InjectUpLinkDataSource>(
        lpgbt,
        InjectUpLinkDataSource::Source::PRBS7,
        link
      );
    } else if (output_choice == 2) {
      bool up = pftool::readline_bool("Count up (y) or down (n)? ", true);
      if (up) {
        injector = std::make_unique<InjectUpLinkDataSource>(
          lpgbt,
          InjectUpLinkDataSource::Source::BIN_CNTR_UP,
          link
        );
      } else {
        injector = std::make_unique<InjectUpLinkDataSource>(
          lpgbt,
          InjectUpLinkDataSource::Source::BIN_CNTR_DOWN,
          link
        );
      }
    } else if (output_choice == 3) {
      uint32_t known_pattern = 0x12345678;
      known_pattern =
        pftool::readline_int("32b const pattern to use: ", known_pattern, true);
      printf("const pattern: 0x%08x\n", known_pattern);
      bool invert = pftool::readline_bool("Invert the constant pattern? ", false);
      if (invert) {
        injector = std::make_unique<InjectUpLinkDataSource>(
          lpgbt,
          InjectUpLinkDataSource::Source::CONST_PATTERN_INV,
          link,
          known_pattern
        );
      } else {
        injector = std::make_unique<InjectUpLinkDataSource>(
          lpgbt,
          InjectUpLinkDataSource::Source::CONST_PATTERN,
          link,
          known_pattern
        );
      }
    }
  }

  pflib::Elinks& elinks = target->elinks();
  std::vector<std::vector<uint32_t>> spy(6);
  int ilink_offset{daq ? 0 : 6};
  printf("word :");
  for (int ilink{0}; ilink < 6; ilink++) {
    spy[ilink] = elinks.spy(ilink_offset + ilink, ilink == 0);
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
      injector->check(spy[ilink]);
    }
  } else {
    injector->check(spy[link]);
  }
}
