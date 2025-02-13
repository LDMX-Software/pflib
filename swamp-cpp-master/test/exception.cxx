#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LPGBT_TEST
#include <boost/test/unit_test.hpp>

#include "carrier.hpp"


void ThrowArgumentError(){
  throw swamp::ArgumentError("ArgumentError");
}

void ThrowFileNotFound(){
  throw swamp::FileNotFound("FileNotFound");
}

void ThrowCorruptedFile(){
  throw swamp::CorruptedFile("CorruptedFile");
}

void ThrowEntryNotFoundError(){
  throw swamp::EntryNotFoundError("EntryNotFoundError");
}

void ThrowBadStatus(){
  throw swamp::BadStatus("BadStatus");
}

void ThrowUnknownChipException(){
  throw swamp::UnknownChipException("UnknownChipException");
}

void ThrowChipConstructorException(){
  throw swamp::ChipConstructorException("ChipConstructorException");
}

void ThrowUnknownTransportException(){
  throw swamp::UnknownTransportException("UnknownTransportException");
}

void ThrowUnknownGPIOException(){
  throw swamp::UnknownGPIOException("UnknownGPIOException");
}

void ThrowGPIOConstructionException(){
  throw swamp::GPIOConstructionException("GPIOConstructionException");
}

void ThrowICECReadTimeoutException(){
  throw swamp::ICECReadTimeoutException("ICECReadTimeoutException");
}

void ThrowICECWriteTimeoutException(){
  throw swamp::ICECWriteTimeoutException("ICECWriteTimeoutException");
}

void ThrowLPGBTI2CDefinitionErrorException(){
  throw swamp::LPGBTI2CDefinitionErrorException("LPGBTI2CDefinitionErrorException");
}

void ThrowLPGBTI2CConfigurationErrorException(){
  throw swamp::LPGBTI2CConfigurationErrorException("LPGBTI2CConfigurationErrorException");
}

void ThrowLPGBTI2CUnlikelyException(){
  throw swamp::LPGBTI2CUnlikelyException("LPGBTI2CUnlikelyException");
}

void ThrowLPGBTI2CSlaveAddressErrorException(){
  throw swamp::LPGBTI2CSlaveAddressErrorException("LPGBTI2CSlaveAddressErrorException");
}

void ThrowLPGBTI2CCommandErrorException(){
  throw swamp::LPGBTI2CCommandErrorException("LPGBTI2CCommandErrorException");
}

void ThrowLPGBTI2CTransactionFailureException(){
  throw swamp::LPGBTI2CTransactionFailureException("LPGBTI2CTransactionFailureException");
}

void ThrowLPGBTGPIOConstructionException(){
  throw swamp::LPGBTGPIOConstructionException("LPGBTGPIOConstructionException");
}

void ThrowECONYAMLConfigErrorException(){
  throw swamp::ECONYAMLConfigErrorException("ECONYAMLConfigErrorException");
}

void ThrowROCYAMLConfigErrorException(){
  throw swamp::ROCYAMLConfigErrorException("ROCYAMLConfigErrorException");
}

// void Throw(){
//   throw swamp::("");
// }

bool is_critical( std::exception const& ex ) { return true; }

BOOST_AUTO_TEST_SUITE (EXCEPTION_TEST)

BOOST_AUTO_TEST_CASE (GENERIC_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowArgumentError(),      swamp::ArgumentError,      is_critical);
  BOOST_CHECK_EXCEPTION(ThrowFileNotFound(),       swamp::FileNotFound,       is_critical);
  BOOST_CHECK_EXCEPTION(ThrowCorruptedFile(),      swamp::CorruptedFile,      is_critical);
  BOOST_CHECK_EXCEPTION(ThrowEntryNotFoundError(), swamp::EntryNotFoundError, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowBadStatus(),          swamp::BadStatus,          is_critical);
}

BOOST_AUTO_TEST_CASE (CHIP_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowUnknownChipException(),     swamp::UnknownChipException,     is_critical);
  BOOST_CHECK_EXCEPTION(ThrowChipConstructorException(), swamp::ChipConstructorException, is_critical);
}

BOOST_AUTO_TEST_CASE (TRANSPORT_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowUnknownTransportException(), swamp::UnknownTransportException, is_critical);
}

BOOST_AUTO_TEST_CASE (GPIO_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowUnknownGPIOException(), swamp::UnknownGPIOException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowGPIOConstructionException(), swamp::GPIOConstructionException, is_critical);
}

BOOST_AUTO_TEST_CASE (IC_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowICECReadTimeoutException(), swamp::ICECReadTimeoutException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowICECWriteTimeoutException(), swamp::ICECWriteTimeoutException, is_critical);
}

BOOST_AUTO_TEST_CASE (LPGBTI2C_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowLPGBTI2CDefinitionErrorException(),    swamp::LPGBTI2CDefinitionErrorException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowLPGBTI2CConfigurationErrorException(), swamp::LPGBTI2CConfigurationErrorException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowLPGBTI2CUnlikelyException(),           swamp::LPGBTI2CUnlikelyException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowLPGBTI2CSlaveAddressErrorException(),  swamp::LPGBTI2CSlaveAddressErrorException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowLPGBTI2CCommandErrorException(),       swamp::LPGBTI2CCommandErrorException, is_critical);
  BOOST_CHECK_EXCEPTION(ThrowLPGBTI2CTransactionFailureException(), swamp::LPGBTI2CTransactionFailureException, is_critical);
}

BOOST_AUTO_TEST_CASE (LPGBTGPIO_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowLPGBTGPIOConstructionException(), swamp::LPGBTGPIOConstructionException, is_critical);
}

BOOST_AUTO_TEST_CASE (ECON_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowECONYAMLConfigErrorException(), swamp::ECONYAMLConfigErrorException, is_critical);
}

BOOST_AUTO_TEST_CASE (ROC_EXCEPTIONS)
{
  BOOST_CHECK_EXCEPTION(ThrowROCYAMLConfigErrorException(), swamp::ROCYAMLConfigErrorException, is_critical);
}

// BOOST_AUTO_TEST_CASE (_EXCEPTIONS)
// {
//   BOOST_CHECK_EXCEPTION(Throw(), swamp::, is_critical);
// }

BOOST_AUTO_TEST_SUITE_END()
