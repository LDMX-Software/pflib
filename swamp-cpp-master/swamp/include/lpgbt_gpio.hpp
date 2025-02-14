#pragma once
#include "gpio.hpp"

#define PIOOUTH_ADDRESS 0x0055
#define PIODIRH_ADDRESS 0x0053
#define PIOINH_ADDRESS 0x01AF

class LPGBT_GPIO : public GPIO
{
public:

  LPGBT_GPIO( const std::string& name, const YAML::Node& config);

  void UpImpl() override;

  void DownImpl() override;

  bool IsUpImpl() override;

  bool IsDownImpl() override;

  void SetDirectionImpl(GPIO_Direction dir) override;

  GPIO_Direction GetDirectionImpl() override;

  void ConfigureImpl(const YAML::Node& node) override;

protected:
  unsigned int m_reg_offset;
  unsigned int m_bit_mask;
};
