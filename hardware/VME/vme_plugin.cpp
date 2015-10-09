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
 *      QpxVmePlugin
 *
 ******************************************************************************/

#include "vme_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"
#include "vmusb.h"

namespace Qpx {

static DeviceRegistrar<QpxVmePlugin> registrar("VME");

QpxVmePlugin::QpxVmePlugin() {

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  controller_ = nullptr;

}


QpxVmePlugin::~QpxVmePlugin() {
  die();
}


bool QpxVmePlugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {

    }
  }
  return true;
}

void QpxVmePlugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxVmePlugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::text) && (q.id_ == "VME/ControllerID")) {
      //PL_DBG << "<VmePlugin> controller expected " << q.value_text;
      if (controller_name_.empty())
        controller_name_ = q.value_text;
    }
  }
  return true;
}

bool QpxVmePlugin::execute_command(Gamma::Setting &set) {
  if (!(status_ & DeviceStatus::can_exec))
    return false;

  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::command) && (q.value_dbl == 1)) {
        q.value_dbl = 0;
        if (q.id_ == "Vme/Iseg/Sing a song")
          return true;
      }
    }
  }

  return false;
}


bool QpxVmePlugin::boot() {
  PL_DBG << "<VmePlugin> Attempting to boot";

  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<VmePlugin> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  if (controller_ != nullptr)
    delete controller_;

  if (controller_name_ == "VmUsb") {
    controller_ = new VmUsb();
    PL_DBG << "<VmePlugin> Controller status: " << controller_->information();
  }

  for (int base_address = 0; base_address < 0xFFFF; base_address += VHS_ADDRESS_SPACE_LENGTH) {
    VmeModule module(controller_, base_address);

    int deviceClass = module.deviceClass();

    if (deviceClass == V12C0 || deviceClass == V24D0) {
      VmeModule *mod = new VmeModule(controller_, base_address);
      modules_.push_back(mod);
      PL_DBG << "<VmePlugin> Adding module [" << base_address << "] firmwareName=" << module.firmwareName() << " serialNumber=" << module.serialNumber();

    }
  }

  status_ = DeviceStatus::loaded | DeviceStatus::booted;
  return true;
}

bool QpxVmePlugin::die() {
  PL_DBG << "<VmePlugin> Disconnecting";

  for (auto &q : modules_)
    delete q;
  modules_.clear();

  if (controller_ != nullptr) {
    delete controller_;
    controller_ = nullptr;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void QpxVmePlugin::get_all_settings() {
}



}
