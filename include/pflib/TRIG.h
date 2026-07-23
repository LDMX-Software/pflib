#ifndef PFLIB_TRIG_H_INCLUDED
#define PFLIB_TRIG_H_INCLUDED 1

#include <stdint.h>

#include <vector>

namespace pflib {

/**
 * Trigger path management interface
 *
 * In full-detector configuration, the trigger path is handled
 * mostly-independently from PFLIB.  However, for teststand
 * and debugging, the trigger path may be integrated into the same
 * FPGA as the DAQ, motivating this software block.
 */
class TRIG {
 public:
  /** Reset the internals */
  virtual void reset() {}

  /** How many elinks are there? */
  virtual int n_elinks() const = 0;

  /** Set up the alignment capture function */
  virtual void setup_alignment_capture(int delay) = 0;

  /** Get the alignment capture function */
  virtual int get_alignment_capture() = 0;

  /** Read the capture block for the given elink */
  virtual std::vector<uint32_t> read_capture_buffer(int ilink) = 0;

  /** Set the BX delay for the given elink */
  virtual void set_bx_delay(int ilink, int delay) = 0;

  /** Get the BX delay for the given elink */
  virtual int get_bx_delay(int ilink) = 0;

  /**
   * @param[in] l1a_per_ror number of L1A sent per ReadOut Request
   * This setting should be copied in from tgt->daq().samples_per_ror().
   */
  void set_l1a_per_ror(int l1a_per_ror);
  int get_l1a_per_ror() const;

  /**
   * Setup the data collection of raw trigger data path
   *
   * @param[in] pipeline how far back in time (in BX) we should start the capture
   * @param[in] econ_id id number for ECON-T we are capturing from
   * @param[in] samples_per_l1a how many samples to caputre per L1A
   * @param[in] presamples number of samples before the BX of interest
   * (i.e. index of sample of interest)
   */
  virtual void setup_daq(int pipeline, int econ_id, int samples_per_l1a,
                         int presamples) = 0;

  /** get the data collection setup */
  virtual void get_daq_setup(int& pipeline, int& econ_id, int& samples_per_l1a,
                             int& presamples) = 0;

  /** Is there a sample available? */
  virtual bool is_sample_available() = 0;

  /** Read the next sample of raw trigger path data */
  virtual std::vector<uint32_t> read_sample() = 0;

  /**
   * Read l1a_per_ror() number of samples and concatenate
   * them into the same buffer
   */
  std::vector<uint32_t> read_event();

  /**
   * configure the trigger algorithm
   *
   * The implementation defines which indices correspond to which
   * parameters since it depends on the target and firmware that
   * is in use.
   */
  virtual void setup_algo(const std::vector<uint32_t>& parameters) = 0;

  /** get the trigger algorithm configuration */
  virtual std::vector<uint32_t> get_algo_setup() = 0;

  /** Is there trigger algorithm output data? */
  virtual bool is_algo_output_available() = 0;

  /**
   * read the next sample of the output of the trigger algorithm
   */
  virtual std::vector<uint32_t> read_algo_output_sample() = 0;

  /**
   * read l1a_per_ror() number of samples of the algo output
   * and concatenate them into the same buffer
   */
  std::vector<uint32_t> read_algo_output();

 private:
  int l1a_per_ror_{0};
};

}  // namespace pflib

#endif  // PFLIB_TRIG_H_INCLUDED
