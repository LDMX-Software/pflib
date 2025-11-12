#include "pflib/Target.h"
#include "pflib/lpgbt/I2C.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/utility/string_format.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/zcu/zcu_optolink.h"

namespace pflib {

static constexpr int ADDR_HCAL_BACKPLANE_DAQ = 0x78 | 0x04;
static constexpr int ADDR_HCAL_BACKPLANE_TRIG = 0x78;
static constexpr int I2C_BUS_ECONS = 0;    // DAQ
static constexpr int I2C_BUS_HGCROCS = 1;  // DAQ
static constexpr int I2C_BUS_BIAS = 1;     // TRIG
static constexpr int I2C_BUS_BOARD = 0;    // TRIG
static constexpr int ADDR_MUX_BIAS = 0x70;
static constexpr int ADDR_MUX_BOARD = 0x71;

/** Currently represents all elinks for dual-link configuration */
class OptoElinksZCU : public Elinks {
 public:
  OptoElinksZCU(lpGBT* lpdaq, lpGBT* lptrig, int itarget)
      : Elinks(6 * 2),
        lp_daq_(lpdaq),
        lp_trig_(lptrig),
        uiodecoder_(
            pflib::utility::string_format("standardLpGBTpair-%d", itarget)) {
    // deactivate all the links except DAQ link 0 for now
    for (int i = 1; i < 6 * 2; i++) markActive(i, false);
  }
  std::vector<uint32_t> spy(int ilink) {
    std::vector<uint32_t> retval;
    // spy now...
    static constexpr int REG_CAPTURE_ENABLE = 16;
    static constexpr int REG_CAPTURE_OLINK = 17;
    static constexpr int REG_CAPTURE_ELINK = 18;
    static constexpr int REG_CAPTURE_PTR = 19;
    static constexpr int REG_CAPTURE_WINDOW = 20;
    uiodecoder_.write(REG_CAPTURE_OLINK, ilink % 6);
    uiodecoder_.write(REG_CAPTURE_ELINK, (ilink / 6 + 1) & 0x7);
    uiodecoder_.write(REG_CAPTURE_ENABLE, 0);
    uiodecoder_.write(REG_CAPTURE_ENABLE, 1);
    usleep(1000);
    uiodecoder_.write(REG_CAPTURE_ENABLE, 0);
    for (int i = 0; i < 64; i++) {
      uiodecoder_.write(REG_CAPTURE_PTR, i);
      usleep(1);
      retval.push_back(uiodecoder_.read(REG_CAPTURE_WINDOW));
    }
    return retval;
  }

  virtual void setBitslip(int ilink, int bitslip) {
    static constexpr int REG_UPLINK_PHASE = 21;
    uint32_t val = uiodecoder_.read(REG_UPLINK_PHASE + ilink / 6);
    uint32_t mask = 0x1F << ((ilink % 6) * 5);
    // set to zero
    val = val | mask;
    val = val ^ mask;
    // mask in new phase
    val = val | ((bitslip & 0x1F) << ((ilink % 6) * 5));
    uiodecoder_.write(REG_UPLINK_PHASE + ilink / 6, val);
  }
  virtual int getBitslip(int ilink) {
    static constexpr int REG_UPLINK_PHASE = 21;
    uint32_t val = uiodecoder_.read(REG_UPLINK_PHASE + (ilink / 6));
    return (val >> ((ilink % 6) * 5)) & 0x1F;
  }
  virtual int scanBitslip(int ilink) { return -1; }
  virtual uint32_t getStatusRaw(int ilink) { return 0; }
  virtual void clearErrorCounters(int ilink) {}
  virtual void resetHard() {
    // not meaningful here
  }

 private:
  lpGBT *lp_daq_, *lp_trig_;
  UIO uiodecoder_;
};

class HcalBackplaneZCU_Capture : public DAQ {
  static constexpr uint32_t ADDR_IDLE_PATTERN = 0x604 / 4;
  static constexpr uint32_t ADDR_HEADER_MARKER = 0x600 / 4;
  static constexpr uint32_t MASK_HEADER_MARKER = 0x0001FF00;
  static constexpr uint32_t ADDR_ENABLE = 0x600 / 4;
  static constexpr uint32_t MASK_ENABLE = 0x00000001;
  static constexpr uint32_t ADDR_EVB_CLEAR = 0x100 / 4;
  static constexpr uint32_t MASK_EVB_CLEAR = 0x00000001;
  static constexpr uint32_t ADDR_ADV_IO = 0x080 / 4;
  static constexpr uint32_t MASK_ADV_IO = 0x00000001;
  static constexpr uint32_t ADDR_ADV_AXIS = 0x080 / 4;
  static constexpr uint32_t MASK_ADV_AXIS = 0x00000002;

  static constexpr uint32_t ADDR_PACKET_SETUP = 0x400 / 4;
  static constexpr uint32_t MASK_ECON_ID = 0x000003FF;
  static constexpr uint32_t MASK_L1A_PER_PACKET = 0x00007C00;
  static constexpr uint32_t MASK_SOI = 0x000F8000;
  static constexpr uint32_t AXIS_ENABLE = 0x80000000;

  static constexpr uint32_t ADDR_UPPER_ADDR = 0x404 / 4;
  static constexpr uint32_t MASK_UPPER_ADDR = 0x0000003F;

  static constexpr uint32_t ADDR_INFO = 0x800 / 4;
  static constexpr uint32_t MASK_IO_NEVENTS = 0x0000007F;
  static constexpr uint32_t MASK_IO_SIZE_NEXT = 0x0000FF80;
  static constexpr uint32_t MASK_AXIS_NWORDS = 0x1FFF0000;
  static constexpr uint32_t MASK_TVALID_DAQ = 0x20000000;
  static constexpr uint32_t MASK_TREADY_DAQ = 0x40000000;

  static constexpr uint32_t ADDR_PAGED_READ = 0x800 / 4;
  static constexpr uint32_t ADDR_BASE_COUNTER = 0x900 / 4;

 public:
  HcalBackplaneZCU_Capture() : DAQ(1), capture_("econd-buffer-0") {
    //    printf("Firmware type and version: %08x %08x
    //    %08x\n",capture_.read(0),capture_.read(ADDR_IDLE_PATTERN),capture_.read(ADDR_HEADER_MARKER));
    // setting up with expected capture parameters
    capture_.write(ADDR_IDLE_PATTERN, 0x1277cc);
    capture_.writeMasked(ADDR_HEADER_MARKER, MASK_HEADER_MARKER,
                         0x1E6);  // 0xAA followed by one bit...
    per_econ_ = true;  // reading from the per-econ buffer, not the AXIS buffer
  }
  virtual void reset() {
    capture_.write(ADDR_EVB_CLEAR, MASK_EVB_CLEAR);  // auto-clear
  }
  virtual int getEventOccupancy() {
    capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR, 0);  // get on page 0
    if (per_econ_)
      return capture_.readMasked(ADDR_INFO, MASK_IO_NEVENTS) /
             samples_per_ror();
    else if (capture_.readMasked(ADDR_INFO, MASK_AXIS_NWORDS) != 0)
      return 1;
    else
      return 0;
  }
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width) {
    // none of these parameters are relevant for the econd capture, which is
    // data-pattern based
  }
  virtual void getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width) {
    l1a_delay = -1;
    l1a_capture_width = -1;
  }
  virtual void bufferStatus(int ilink, bool& empty, bool& full) {
    int nevt = getEventOccupancy();
    empty = (nevt == 0);
    full = (nevt == 0x7f);
  }
  virtual void setup(int econid, int samples_per_ror, int soi) {
    pflib::DAQ::setup(econid, samples_per_ror, soi);
    capture_.writeMasked(ADDR_PACKET_SETUP, MASK_ECON_ID, econid);
    capture_.writeMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET,
                         samples_per_ror);
    capture_.writeMasked(ADDR_PACKET_SETUP, MASK_SOI, soi);
  }
  virtual void enable(bool doenable) {
    if (doenable)
      capture_.rmw(ADDR_ENABLE, MASK_ENABLE, MASK_ENABLE);
    else
      capture_.rmw(ADDR_ENABLE, MASK_ENABLE, 0);
    if (!per_econ_ && doenable)
      capture_.rmw(ADDR_PACKET_SETUP, AXIS_ENABLE, AXIS_ENABLE);
    else
      capture_.rmw(ADDR_PACKET_SETUP, AXIS_ENABLE, 0);
    pflib::DAQ::enable(doenable);
  }
  virtual bool enabled() {
    return capture_.readMasked(ADDR_ENABLE, MASK_ENABLE);
  }
  virtual std::vector<uint32_t> getLinkData(int ilink) {
    uint32_t words = 0;
    std::vector<uint32_t> retval;
    static const uint32_t UBITS = 0x3F00;
    static const uint32_t LBITS = 0x00FF;

    capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR,
                         0);  // must be on basic page

    if (per_econ_)
      words = capture_.readMasked(ADDR_INFO, MASK_IO_SIZE_NEXT);
    else
      words = capture_.readMasked(ADDR_INFO, MASK_AXIS_NWORDS);

    uint32_t iold = 0xFFFFFF;
    for (uint32_t i = 0; i < words; i++) {
      if ((iold & UBITS) != (i & UBITS))  // new upper address block
        if (per_econ_)
          capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR,
                               (i >> 8) | 0x04);
        else
          capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR,
                               (i >> 8) | 0x20);
      retval.push_back(capture_.read(ADDR_PAGED_READ + (i & LBITS)));
    }
    return retval;
  }
  virtual void advanceLinkReadPtr() {
    if (per_econ_)
      capture_.write(ADDR_ADV_IO, MASK_ADV_IO);  // auto-clear
    else
      capture_.write(ADDR_ADV_AXIS, MASK_ADV_AXIS);  // auto-clear
  }

  virtual std::map<std::string, uint32_t> get_debug(uint32_t ask) {
    std::map<std::string, uint32_t> dbg;
    capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR, 0);
    static const int stepsize = 1;
    FILE* f = fopen("dump.txt", "w");

    for (int i = 0; i < 0xFFF / 4; i++)
      fprintf(f, "%03x %03x %08x\n", i * 4, i, capture_.read(i));
    fclose(f);

    dbg["COUNT_IDLES"] = capture_.read(ADDR_BASE_COUNTER);
    dbg["COUNT_NONIDLES"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 1);
    dbg["COUNT_STARTS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 2);
    dbg["COUNT_STOPS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 3);
    dbg["COUNT_WORDS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 4);
    dbg["COUNT_IO_ADV"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 5);
    dbg["COUNT_TLAST"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 6);
    dbg["QUICKSPY"] = capture_.read(ADDR_BASE_COUNTER + 0x10);
    dbg["STATE"] = capture_.read(ADDR_BASE_COUNTER + 0x11);
    return dbg;
  }

 private:
  UIO capture_;
  bool per_econ_;
};

class HcalBackplaneZCU : public Hcal {
 public:
  HcalBackplaneZCU(int itarget, uint8_t board_mask) {
    // first, setup the optical links
    std::string uio_coder =
        pflib::utility::string_format("standardLpGBTpair-%d", itarget);

    daq_tport_ = std::make_unique<pflib::zcu::lpGBT_ICEC_Simple>(
        uio_coder, false, ADDR_HCAL_BACKPLANE_DAQ);
    trig_tport_ = std::make_unique<pflib::zcu::lpGBT_ICEC_Simple>(
        uio_coder, true, ADDR_HCAL_BACKPLANE_TRIG);
    daq_lpgbt_ = std::make_unique<pflib::lpGBT>(*daq_tport_);
    trig_lpgbt_ = std::make_unique<pflib::lpGBT>(*trig_tport_);

    // next, create the Hcal I2C objects
    econ_i2c_ = std::make_shared<pflib::lpgbt::I2C>(*daq_lpgbt_, I2C_BUS_ECONS);
    econ_i2c_->set_bus_speed(1000);
    add_econ(0, 0x60 | 0, "econd", econ_i2c_);
    add_econ(1, 0x20 | 0, "econt", econ_i2c_);
    add_econ(2, 0x20 | 1, "econt", econ_i2c_);

    roc_i2c_ =
        std::make_shared<pflib::lpgbt::I2C>(*daq_lpgbt_, I2C_BUS_HGCROCS);
    roc_i2c_->set_bus_speed(1000);
    for (int ibd = 0; ibd < 4; ibd++) {
      if ((board_mask & (1 << ibd)) == 0) continue;
      std::shared_ptr<pflib::I2C> bias_i2c =
          std::make_shared<pflib::lpgbt::I2CwithMux>(*trig_lpgbt_, I2C_BUS_BIAS,
                                                     ADDR_MUX_BIAS, (1 << ibd));
      std::shared_ptr<pflib::I2C> board_i2c =
          std::make_shared<pflib::lpgbt::I2CwithMux>(
              *trig_lpgbt_, I2C_BUS_BOARD, ADDR_MUX_BOARD, (1 << ibd));
      // TODO allow for board->typ_version configuration to be passed here
      // right now its hardcoded because everyone has one of these
      // but we could modify this constructor and its calling factory
      // function in order to pass in a configuration

      add_roc(ibd, 0x28 | (ibd * 8), "sipm_rocv3b", roc_i2c_, bias_i2c,
              board_i2c);
    }

    elinks_ = std::make_unique<OptoElinksZCU>(&(*daq_lpgbt_), &(*trig_lpgbt_),
                                              itarget);
    daq_ = std::make_unique<HcalBackplaneZCU_Capture>();

    pflib::lpgbt::standard_config::setup_hcal_trig(*trig_lpgbt_);
    pflib::lpgbt::standard_config::setup_hcal_daq(*daq_lpgbt_);
  }

  virtual void softResetROC(int which) override {
    // assuming everything was done with the standard config
    if (which < 0 || which == 0) {
      trig_lpgbt_->gpio_interface().setGPO("HGCROC0_SRST", false);
      trig_lpgbt_->gpio_interface().setGPO("HGCROC0_SRST", true);
    }
    if (which < 0 || which == 1) {
      daq_lpgbt_->gpio_interface().setGPO("HGCROC1_SRST", false);
      daq_lpgbt_->gpio_interface().setGPO("HGCROC1_SRST", true);
    }
    if (which < 0 || which == 2) {
      daq_lpgbt_->gpio_interface().setGPO("HGCROC2_SRST", false);
      daq_lpgbt_->gpio_interface().setGPO("HGCROC2_SRST", true);
    }
    if (which < 0 || which == 3) {
      trig_lpgbt_->gpio_interface().setGPO("HGCROC3_SRST", false);
      trig_lpgbt_->gpio_interface().setGPO("HGCROC3_SRST", true);
    }
  }

  virtual void softResetECON(int which = -1) override {
    trig_lpgbt_->gpio_interface().setGPO("ECON_SRST", false);
    trig_lpgbt_->gpio_interface().setGPO("ECON_SRST", true);
  }

  virtual void hardResetROCs() override {
    trig_lpgbt_->gpio_interface().setGPO("HGCROC0_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC0_HRST", true);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC1_HRST", false);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC1_HRST", true);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC2_HRST", false);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC2_HRST", true);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC3_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC3_HRST", true);
  }

  virtual void hardResetECONs() override {
    trig_lpgbt_->gpio_interface().setGPO("ECON_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("ECON_HRST", true);
  }

  virtual Elinks& elinks() override { return *elinks_; }

  virtual DAQ& daq() override { return *daq_; }

 private:
  /// let the target that holds this Hcal see our members
  friend class HcalBackplaneZCUTarget;
  std::unique_ptr<pflib::zcu::lpGBT_ICEC_Simple> daq_tport_, trig_tport_;
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::shared_ptr<pflib::I2C> roc_i2c_, econ_i2c_;
  std::unique_ptr<OptoElinksZCU> elinks_;
  std::unique_ptr<HcalBackplaneZCU_Capture> daq_;
};

class HcalBackplaneZCUTarget : public Target {
 public:
  HcalBackplaneZCUTarget(int ilink, uint8_t board_mask) : Target() {
    zcuhcal_ = std::make_shared<HcalBackplaneZCU>(ilink, board_mask);
    hcal_ = zcuhcal_;

    // copy I2C connections into Target
    // in case user wants to do raw I2C transactions for testing
    for (auto [bid, conn] : zcuhcal_->roc_connections_) {
      i2c_[pflib::utility::string_format("HGCROC_%d", bid)] = conn.roc_i2c_;
      i2c_[pflib::utility::string_format("BOARD_%d", bid)] = conn.board_i2c_;
      i2c_[pflib::utility::string_format("BIAS_%d", bid)] = conn.bias_i2c_;
    }
    for (auto [bid, conn] : zcuhcal_->econ_connections_) {
      i2c_[pflib::utility::string_format("ECON_%d", bid)] = conn.i2c_;
    }

    fc_ = std::shared_ptr<FastControl>(make_FastControlCMS_MMap());
  }

  virtual void setup_run(int irun, Target::DaqFormat format, int contrib_id) {
    format_ = format;
    contrib_id_ = contrib_id;
  }

  virtual std::vector<uint32_t> read_event() override {
    std::vector<uint32_t> buf;

    if (format_ == Target::DaqFormat::ECOND_SW_HEADERS) {
      for (int ievt = 0; ievt < zcuhcal_->daq().samples_per_ror(); ievt++) {
        std::vector<uint32_t> subpacket =
            zcuhcal_->daq().getLinkData(0);  // only one elink right now
        buf.push_back((0x1 << 28) | ((zcuhcal_->daq().econid() & 0xFFF) << 18) |
                      (ievt << 13) |
                      ((ievt == zcuhcal_->daq().soi()) ? (1 << 12) : (0)) |
                      (subpacket.size()));
        buf.insert(buf.end(), subpacket.begin(), subpacket.end());
        zcuhcal_->daq().advanceLinkReadPtr();
      }
    } else {
      PFEXCEPTION_RAISE("NoImpl",
                        "HcalBackplaneZCUTarget::read_event not implemented "
                        "for provided DaqFormat");
    }

    return buf;
  }

 private:
  Target::DaqFormat format_;
  int contrib_id_;
  std::shared_ptr<HcalBackplaneZCU> zcuhcal_;
};

Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask) {
  return new HcalBackplaneZCUTarget(ilink, board_mask);
}

}  // namespace pflib
