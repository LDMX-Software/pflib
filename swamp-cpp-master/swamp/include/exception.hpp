#ifndef __SWAMP_EXCEPTION_HPP__
#define __SWAMP_EXCEPTION_HPP__


#include <exception>
#include <string>
#include <vector>



namespace swamp {

  class ExceptionHelper : public std::exception
  {
  public:
    ExceptionHelper(const char *msg)
      : std::exception(), msg_(msg)
    { }
    virtual const char * what() const throw() { return msg_; }
  private:
    const char *msg_;
  };
  
#define SWAMP_DEFINE_EXCEPTION(ClassName,Message)	\
  class ClassName : public ExceptionHelper		\
  {							\
  public:						\
  ClassName(const char *msg = Message)			\
    : ExceptionHelper(msg)			\
    { }						\
  };


  // Generic exceptions
  SWAMP_DEFINE_EXCEPTION(ArgumentError, "Exception class to handle argument errors")
  SWAMP_DEFINE_EXCEPTION(FileNotFound, "File was not found");
  SWAMP_DEFINE_EXCEPTION(CorruptedFile, "File was corrupted");
  SWAMP_DEFINE_EXCEPTION(EntryNotFoundError, "Entry not found");
  SWAMP_DEFINE_EXCEPTION(BadStatus, "Bad value read from status registers");
  SWAMP_DEFINE_EXCEPTION(MethodNotImplementedException, "Exception class to handle wrong calls to method that are not implemented");

  //CHIP exception
  SWAMP_DEFINE_EXCEPTION(UnknownChipException, "Exception class to handle unknown chip type");
  SWAMP_DEFINE_EXCEPTION(ChipConstructorException, "Exception class to handle wrong chip configuration (in constructor)");

  //TRANSPORT exception
  SWAMP_DEFINE_EXCEPTION(UnknownTransportException, "Exception class to handle unknown transport type");
  SWAMP_DEFINE_EXCEPTION(TransportConstructorException, "Exception class to handle wrong transport configuration (in constructor)");

  //GPIO exception
  SWAMP_DEFINE_EXCEPTION(UnknownGPIOException, "Exception class to handle unknown gpio type");
  SWAMP_DEFINE_EXCEPTION(GPIOConstructionException, "Exception class to handle wrong gpio configuration (in constructor)");
  
  // IC Exceptions
  SWAMP_DEFINE_EXCEPTION(ICECReadTimeoutException, "Exception class to handle IC read timeout");
  SWAMP_DEFINE_EXCEPTION(ICECWriteTimeoutException, "Exception class to handle IC write timeout");

  // LPGBT_I2C Exceptions
  SWAMP_DEFINE_EXCEPTION(LPGBTI2CDefinitionErrorException, "Exception class to handle LPGBT_I2C definition error")
  SWAMP_DEFINE_EXCEPTION(LPGBTI2CConfigurationErrorException, "Exception class to handle LPGBT_I2C configuration error")
  SWAMP_DEFINE_EXCEPTION(LPGBTI2CUnlikelyException, "Exception class that should never been thrown. NEED DEBUG")
  SWAMP_DEFINE_EXCEPTION(LPGBTI2CSlaveAddressErrorException, "Exception class to handle LPGBT_I2C slave address error")
  SWAMP_DEFINE_EXCEPTION(LPGBTI2CCommandErrorException, "Exception class to handle LPGBT_I2C command error")
  SWAMP_DEFINE_EXCEPTION(LPGBTI2CTransactionFailureException, "Exception class to handle LPGBT_I2C transaction failure")

  // LPGBT_GPIO Exceptions
  SWAMP_DEFINE_EXCEPTION(LPGBTGPIOConstructionException, "Exception class to handle wrong lpgbt gpio configuration (in constructor)")
  
  // ECON Exceptions
  SWAMP_DEFINE_EXCEPTION(ECONConstructorErrorException, "Exception class to handle errors in construction of ECON objects")
  SWAMP_DEFINE_EXCEPTION(ECONYAMLConfigErrorException, "Exception class to handle wrong yaml configuration format for ECON objects")

  // ROC Exceptions
  SWAMP_DEFINE_EXCEPTION(ROCYAMLConfigErrorException, "Exception class to handle wrong yaml configuration format for ROC objects")

  // ADC Exceptions
  SWAMP_DEFINE_EXCEPTION(UnknownADCException, "Exception class to handle unknown ADC type")

  // LPGBT_ADC Exceptions
  SWAMP_DEFINE_EXCEPTION(LPGBTADCYAMLMissingParameterException, "Exception class to handle missing parameter configuration for LPGBT_ADC objects")
  SWAMP_DEFINE_EXCEPTION(LPGBTADCYAMLConfigErrorException, "Exception class to handle wrong yaml configuration format for LPGBT_ADC objects")
  SWAMP_DEFINE_EXCEPTION(LPGBTADCConvertTimeoutException, "Exception class to handle conversion timeout for LPGBT_ADC objects")
}

#endif
