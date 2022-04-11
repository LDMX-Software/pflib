#ifndef PFTOOL_I2C_H
#define PFTOOL_I2C_H
#include "Menu.h"
#include "pflib/PolarfireTarget.h"
using pflib::PolarfireTarget;

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
void i2c(const std::string &cmd, PolarfireTarget *pft);

/**
 * Low-level Wishbone Interactions
 *
 * We acces the active wishbone connection via PolarfireTarget.
 * @see BaseMenu::readline_int for how integers can be passed in
 * (in short, decimal, hex, octal, and binary are supported)
 *
 * ## Commands
 * - RESET : WishboneInterface::wb_reset
 * - WRITE : WishboneInterface::wb_write
 * - READ : WishboneInterface::wb_read
 * - BLOCKREAD : Loop over READ for input number of words
 * - STATUS : Print results form WishboneInterface::wb_errors
 *
 * @param[in] cmd Wishbone Command
 * @param[in] pft active target
 */
void wb(const std::string &cmd, PolarfireTarget *pft);
#endif /* PFTOOL_I2C_H */
