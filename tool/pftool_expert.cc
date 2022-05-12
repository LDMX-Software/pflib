#include "pftool_expert.h"

void i2c( const std::string& cmd, PolarfireTarget* pft )
{
  static uint32_t addr=0;
  static int waddrlen=1;
  static int i2caddr=0;
  pflib::I2C i2c(pft->wb);

  if (cmd=="BUS") {
    int ibus=i2c.get_active_bus();
    ibus=BaseMenu::readline_int("Bus to make active",ibus);
    i2c.set_active_bus(ibus);
  }
  if (cmd=="WRITE") {
    i2caddr=BaseMenu::readline_int("I2C Target ",i2caddr);
    uint32_t val=BaseMenu::readline_int("Value ",0);
    i2c.set_bus_speed(1000);
    i2c.write_byte(i2caddr,val);
  }
  if (cmd=="READ") {
    i2caddr=BaseMenu::readline_int("I2C Target",i2caddr);
    i2c.set_bus_speed(100);
    uint8_t val=i2c.read_byte(i2caddr);
    printf("%02x : %02x\n",i2caddr,val);
  }
  if (cmd=="MULTIREAD") {
    i2caddr=BaseMenu::readline_int("I2C Target",i2caddr);
    waddrlen=BaseMenu::readline_int("Read address length",waddrlen);
    std::vector<uint8_t> waddr;
    if (waddrlen>0) {
      addr=BaseMenu::readline_int("Read address",addr);
      for (int i=0; i<waddrlen; i++) {
        waddr.push_back(uint8_t(addr&0xFF));
        addr=(addr>>8);
      }
    }
    int len=BaseMenu::readline_int("Read length",1);
    std::vector<uint8_t> data=i2c.general_write_read(i2caddr,waddr);
    for (size_t i=0; i<data.size(); i++)
      printf("%02x : %02x\n",int(i),data[i]);
  }
  if (cmd=="MULTIWRITE") {
    i2caddr=BaseMenu::readline_int("I2C Target",i2caddr);
    waddrlen=BaseMenu::readline_int("Write address length",waddrlen);
    std::vector<uint8_t> wdata;
    if (waddrlen>0) {
      addr=BaseMenu::readline_int("Write address",addr);
      for (int i=0; i<waddrlen; i++) {
        wdata.push_back(uint8_t(addr&0xFF));
        addr=(addr>>8);
      }
    }
    int len=BaseMenu::readline_int("Write data length",1);
    for (int j=0; j<len; j++) {
      char prompt[64];
      sprintf(prompt,"Byte %d: ",j);
      int id=BaseMenu::readline_int(prompt,0);
      wdata.push_back(uint8_t(id));
    }
    i2c.general_write_read(i2caddr,wdata);
  }
}

void wb( const std::string& cmd, PolarfireTarget* pft )
{
  static uint32_t target=0;
  static uint32_t addr=0;
  if (cmd=="RESET") {
    pft->wb->wb_reset();
  }
  if (cmd=="WRITE") {
    target=BaseMenu::readline_int("Target",target);
    addr=BaseMenu::readline_int("Address",addr);
    uint32_t val=BaseMenu::readline_int("Value",0);
    pft->wb->wb_write(target,addr,val);
  }
  if (cmd=="READ") {
    target=BaseMenu::readline_int("Target",target);
    addr=BaseMenu::readline_int("Address",addr);
    uint32_t val=pft->wb->wb_read(target,addr);
    printf(" Read: 0x%08x\n",val);
  }
  if (cmd=="BLOCKREAD") {
    target=BaseMenu::readline_int("Target",target);
    addr=BaseMenu::readline_int("Starting address",addr);
    int len=BaseMenu::readline_int("Number of words",8);
    for (int i=0; i<len; i++) {
      uint32_t val=pft->wb->wb_read(target,addr+i);
      printf(" %2d/0x%04x : 0x%08x\n",target,addr+i,val);
    }
  }
  if (cmd=="STATUS") {
    uint32_t crcd, crcu, wbe;
    pft->wb->wb_errors(crcu,crcd,wbe);
    printf("CRC errors: Up -- %d   Down --%d \n",crcu,crcd);
    printf("Wishbone errors: %d\n",wbe);
  }
}

void link( const std::string& cmd, PolarfireTarget* pft ) {
  if (cmd=="STATUS") {
    uint32_t status;
    uint32_t ratetx=0, raterx=0, ratek=0;
    //    uint32_t status=ldmx->link_status(&ratetx,&raterx,&ratek);
    printf("Link Status: %08x   TX Rate: %d   RX Rate: %d   K Rate : %d\n",status,ratetx,raterx,ratek);
  }
  if (cmd=="CONFIG") {
    bool reset_txpll=BaseMenu::readline_bool("TX-PLL Reset?", false);
    bool reset_gtxtx=BaseMenu::readline_bool("GTX-TX Reset?", false);
    bool reset_tx=BaseMenu::readline_bool("TX Reset?", false);
    bool reset_rx=BaseMenu::readline_bool("RX Reset?", false);
    static bool polarity=false;
    polarity=BaseMenu::readline_bool("Choose negative TX polarity?",polarity);
    static bool polarity2=false;
    polarity2=BaseMenu::readline_bool("Choose negative RX polarity?",polarity2);
    static bool bypass=false;
    bypass=BaseMenu::readline_bool("Bypass CRC on receiver?",false);
    //    ldmx->link_setup(polarity,polarity2,bypass);
    // ldmx->link_resets(reset_txpll,reset_gtxtx,reset_tx,reset_rx);
  }
  if (cmd=="SPY") {
    /*
    std::vector<uint32_t> spy=ldmx->link_spy();
    for (size_t i=0; i<spy.size(); i++)
      printf("%02d %05x\n",int(i),spy[i]);
    */
  }
}

namespace {
auto menu_expert = pftool::menu("EXPERT", "expert functions");

auto menu_wishbone = menu_expert->submenu("WB", "raw wishbone interactions")
  ->line("RESET", "Enable/disable (toggle)",  wb )
  ->line("READ", "Read from an address",  wb )
  ->line("BLOCKREAD", "Read several words starting at an address",  wb )
  ->line("WRITE", "Write to an address",  wb )
  ->line("STATUS", "Wishbone errors counters",  wb )
;

auto menu_i2c = menu_expert->submenu("I2C", "raw I2C interactions")
  ->line("BUS","Pick the I2C bus to use", i2c )
  ->line("READ", "Read from an address",  i2c )
  ->line("WRITE", "Write to an address",  i2c )
  ->line("MULTIREAD", "Read from an address",  i2c )
  ->line("MULTIWRITE", "Write to an address",  i2c )
;

auto menu_olink = menu_expert->submenu("OLINK","optical link interactions (NOT IMPLEMENTED)")
      /*
  ->line("STATUS","Dump link status", link )
  ->line("CONFIG","Setup link", link )
  ->line("SPY", "Spy on the uplink",  link )
    */
;
}
