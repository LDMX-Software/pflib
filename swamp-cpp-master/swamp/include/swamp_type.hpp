#pragma once
#include <type_traits>

template <typename E>
struct FEnableBitmaskOperators {
    static constexpr bool enable = false;
};

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator|(
    E Lhs, E Rhs) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(Lhs) |
                          static_cast<std::underlying_type_t<E>>(Rhs));
}

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator&(
    E Lhs, E Rhs) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(Lhs) &
                          static_cast<std::underlying_type_t<E>>(Rhs));
}

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator^(
    E Lhs, E Rhs) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(Lhs) ^
                          static_cast<std::underlying_type_t<E>>(Rhs));
}

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator~(
    E Lhs) {
    return static_cast<E>(~static_cast<std::underlying_type_t<E>>(Lhs));
}

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator|=(
    E Lhs, E Rhs) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(Lhs) |=
                          static_cast<std::underlying_type_t<E>>(Rhs));
}

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator&=(
    E Lhs, E Rhs) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(Lhs) &=
                          static_cast<std::underlying_type_t<E>>(Rhs));
}

template <typename E>
typename std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator^=(
    E Lhs, E Rhs) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(Lhs) ^=
                          static_cast<std::underlying_type_t<E>>(Rhs));
}


namespace swamp{
  enum class ChipType;
  enum class GPIOType;
  enum class TransportType;
}

template <>
struct FEnableBitmaskOperators<swamp::ChipType> {
    static constexpr bool enable = true;
};

template <>
struct FEnableBitmaskOperators<swamp::GPIOType> {
    static constexpr bool enable = true;
};

template <>
struct FEnableBitmaskOperators<swamp::TransportType> {
    static constexpr bool enable = true;
};

namespace swamp{

  enum class ChipType : int {
    GENERIC     = 0x00,
    LPGBT       = 0x08,
    LPGBT_D     = LPGBT | 0x1,
    LPGBT_T     = LPGBT | 0x2,
    LPGBT_D2    = LPGBT | 0x4,
    HGCROC      = LPGBT << 0x3,
    HGCROC_Si   = HGCROC | (LPGBT << 0x1),
    HGCROC_SiPM = HGCROC | (LPGBT << 0x2),
    ECON        = HGCROC << 0x5,
    ECON_D      = ECON | (HGCROC << 0x1),
    ECON_T      = ECON | (HGCROC << 0x2),
    VTRXP       = ECON << 0x1,
    GBT_SCA     = VTRXP << 0x1,
  };

  enum class GPIOType : int {
    GENERIC      = 0x00,
    RSTB         = 0x01,
    PWR_EN       = RSTB << 0x1,
    PWR_GD       = PWR_EN << 0x3,
    PWR_GD_DCDC  = PWR_GD | (PWR_EN << 0x1),
    PWR_GD_LDO   = PWR_GD | (PWR_EN << 0x2),
    HGCROC_RE    = PWR_GD << 0x3,
    HGCROC_RE_SB = HGCROC_RE | (PWR_GD << 0x1),
    HGCROC_RE_HB = HGCROC_RE | (PWR_GD << 0x2),
    ECON_RE      = HGCROC_RE << 0x3,
    ECON_RE_SB   = ECON_RE | (HGCROC_RE << 0x1),
    ECON_RE_HB   = ECON_RE | (HGCROC_RE << 0x2),
    READY        = ECON_RE << 0x1,
  };

  enum class TransportType : int {
    GENERIC       = 0x00,
    IC            = 0x01,
    EC            = IC << 0x1,
    LPGBT_I2C     = EC << 0x3,
    LPGBT_I2C_DAQ = LPGBT_I2C | (EC << 0x1),
    LPGBT_I2C_TRG = LPGBT_I2C | (EC << 0x2),
    GBT_SCA_I2C   = LPGBT_I2C << 0x1,
  };
}
