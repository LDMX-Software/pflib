#include "tool.h"

/**
 * Direct I2C commands
 *
 * We construct a raw I2C interface by passing the wishbone from the target
 * into the pflib::I2C class.
 *
 * ## Commands
 * - BUS : pflib::I2C::set_active_bus, default retrieved by
 * pflib::I2C::get_active_bus
 * - WRITE : pflib::I2C::write_byte after passing 1000 to
 * pflib::I2C::set_bus_speed
 * - READ : pflib::I2C::read_byte after passing 100 to pflib::I2C::set_bus_speed
 * - MULTIREAD : pflib::I2C::general_write_read
 * - MULTIWRITE : pflib::I2C::general_write_read
 *
 * @param[in] cmd I2C command
 * @param[in] pft active target
 */
static void i2c(const std::string& cmd, Target* target) {
  static uint32_t addr = 0;
  static int waddrlen = 1;
  static int i2caddr = 0;
  static std::string chosen_bus;

  printf(" Current bus: %s\n", chosen_bus.c_str());

  if (cmd == "BUS") {
    std::vector<std::string> busses = target->i2c_bus_names();
    printf("\n\nKnown I2C busses:\n");
    for (auto name : target->i2c_bus_names()) printf(" %s\n", name.c_str());
    chosen_bus = pftool::readline("Bus to make active: ", chosen_bus);
    if (chosen_bus.size() > 1) target->get_i2c_bus(chosen_bus);
  }
  if (chosen_bus.size() < 2) return;
  pflib::I2C& i2c = target->get_i2c_bus(chosen_bus);
  if (cmd == "WRITE") {
    i2caddr = pftool::readline_int("I2C Target ", i2caddr);
    uint32_t val = pftool::readline_int("Value ", 0);
    i2c.set_bus_speed(1000);
    i2c.write_byte(i2caddr, val);
  }
  if (cmd == "READ") {
    i2caddr = pftool::readline_int("I2C Target", i2caddr);
    i2c.set_bus_speed(100);
    uint8_t val = i2c.read_byte(i2caddr);
    printf("%02x : %02x\n", i2caddr, val);
  }
  if (cmd == "MULTIREAD") {
    i2caddr = pftool::readline_int("I2C Target", i2caddr);
    waddrlen = pftool::readline_int("Read address length", waddrlen);
    std::vector<uint8_t> waddr;
    if (waddrlen > 0) {
      addr = pftool::readline_int("Read address", addr);
      for (int i = 0; i < waddrlen; i++) {
        waddr.push_back(uint8_t(addr & 0xFF));
        addr = (addr >> 8);
      }
    }
    int len = pftool::readline_int("Read length", 1);
    std::vector<uint8_t> data = i2c.general_write_read(i2caddr, waddr);
    for (size_t i = 0; i < data.size(); i++)
      printf("%02x : %02x\n", int(i), data[i]);
  }
  if (cmd == "MULTIWRITE") {
    i2caddr = pftool::readline_int("I2C Target", i2caddr);
    waddrlen = pftool::readline_int("Write address length", waddrlen);
    std::vector<uint8_t> wdata;
    if (waddrlen > 0) {
      addr = pftool::readline_int("Write address", addr);
      for (int i = 0; i < waddrlen; i++) {
        wdata.push_back(uint8_t(addr & 0xFF));
        addr = (addr >> 8);
      }
    }
    int len = pftool::readline_int("Write data length", 1);
    for (int j = 0; j < len; j++) {
      char prompt[64];
      sprintf(prompt, "Byte %d: ", j);
      int id = pftool::readline_int(prompt, 0);
      wdata.push_back(uint8_t(id));
    }
    i2c.general_write_read(i2caddr, wdata);
  }
}

namespace {
auto menu_expert = pftool::menu("EXPERT", "expert functions");
auto menu_i2c = menu_expert->submenu("I2C", "raw I2C interactions")
                    ->line("BUS", "Pick the I2C bus to use", i2c)
                    ->line("READ", "Read from an address", i2c)
                    ->line("WRITE", "Write to an address", i2c)
                    ->line("MULTIREAD", "Read from an address", i2c)
                    ->line("MULTIWRITE", "Write to an address", i2c);

}
