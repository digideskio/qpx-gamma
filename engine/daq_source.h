/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Qpx::Source abstract class
 *
 ******************************************************************************/

#ifndef QPX_DAQ_SOURCE
#define QPX_DAQ_SOURCE

#include "generic_setting.h"
#include "synchronized_queue.h"
#include "spill.h"
#include "custom_logger.h"

namespace Qpx {

enum SourceStatus {
  dead      = 0,
  loaded    = 1 << 0,
  booted    = 1 << 1,
  can_boot  = 1 << 2,
  can_run   = 1 << 3,
  can_oscil = 1 << 4
};

inline SourceStatus operator|(SourceStatus a, SourceStatus b) {return static_cast<SourceStatus>(static_cast<int>(a) | static_cast<int>(b));}
inline SourceStatus operator&(SourceStatus a, SourceStatus b) {return static_cast<SourceStatus>(static_cast<int>(a) & static_cast<int>(b));}
inline SourceStatus operator^(SourceStatus a, SourceStatus b) {return static_cast<SourceStatus>(static_cast<int>(a) ^ static_cast<int>(b));}


class Source {

public:

  Source() : status_(SourceStatus(0)) {}
  virtual ~Source() {}

  static std::string plugin_name() {return std::string();}

  virtual std::string device_name() const {return std::string();}

  bool load_setting_definitions(std::string file);
  bool save_setting_definitions(std::string file);


  SourceStatus status() const {return status_;}
  virtual bool boot() {return false;}
  virtual bool die() {status_ = SourceStatus(0); return true;}

  virtual bool write_settings_bulk(Qpx::Setting &set) {return false;}
  virtual bool read_settings_bulk(Qpx::Setting &set) const {return false;}
  virtual void get_all_settings() {}

  virtual std::list<Hit> oscilloscope() {return std::list<Hit>();}

  virtual bool daq_init() {return true;}
  virtual bool daq_start(SynchronizedQueue<Spill*>* out_queue) {return false;}
  virtual bool daq_stop() {return true;}
  virtual bool daq_running() {return false;}

private:
  //no copying
  void operator=(Source const&);
  Source(const Source&);


protected:
  SourceStatus                            status_;
  std::map<std::string, Qpx::SettingMeta> setting_definitions_;
  std::string                             profile_path_;

};

typedef std::shared_ptr<Source> SourcePtr;

}

#endif
