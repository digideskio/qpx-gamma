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
 *      Qpx::Spectrum::Spectrum2D spectrum type for true coincidence with
 *      two dimensions of energy.
 *
 ******************************************************************************/


#include <fstream>
#include "spectrum2D.h"
#include "custom_logger.h"
#include "custom_timer.h"

namespace Qpx {
namespace Spectrum {

static Registrar<Spectrum2D> registrar("2D");

bool Spectrum2D::initialize() {
  //make add pattern has exactly 2 channels
  //match pattern can be whatever
  
  int adds = 0;
  for (auto &q : metadata_.add_pattern)
    if (q == 1)
      adds++;

  if (adds != 2) {
    PL_WARN << "Invalid 2D spectrum created (bad add pattern)";
    return false;
  }
  
  metadata_.resolution = pow(2, 16 - shift_by_);
  metadata_.dimensions = 2;
  energies_.resize(2);
  pattern_.resize(2, 0);
  buffered_ = (get_attr("buffered").value != 0);

  adds = 0;
  for (int i=0; i < metadata_.add_pattern.size(); i++) {
    if (metadata_.add_pattern[i] == 1) {
      pattern_[adds] = i;
      adds++;
    }
  }
  metadata_.type = my_type();
  return true;
}

void Spectrum2D::_set_detectors(const std::vector<Gamma::Detector>& dets) {
  metadata_.detectors.clear();

  if (dets.size() >= 2) {
    int total = metadata_.add_pattern.size();
    if (dets.size() < total)
      total = dets.size();
    metadata_.detectors.resize(metadata_.dimensions, Gamma::Detector());

    int j=0;
    for (int i=0; i < total; ++i) {
      if (metadata_.add_pattern[i]) {
        metadata_.detectors[j] = dets[i];
        j++;
        if (j >= metadata_.dimensions)
          j = metadata_.dimensions - 1;
      }
    }
  }

  this->recalc_energies();
}

void Spectrum2D::_add_bulk(const Entry& e) {
  if ((e.first.size() == 2) && (e.first[0] < metadata_.resolution) && (e.first[1] < metadata_.resolution)) {
    spectrum_[std::pair<uint16_t,uint16_t>(e.first[0], e.first[1])] += e.second;
    metadata_.total_count += e.second;
  }
}

uint64_t Spectrum2D::_get_count(std::initializer_list<uint16_t> list) const {
  if (list.size() != 2)
    return 0;

  std::vector<uint16_t> coords(list.begin(), list.end());

  if ((coords[0] >= metadata_.resolution) || (coords[1] >= metadata_.resolution))
    return 0;
  
  std::pair<uint16_t,uint16_t> point(coords[0], coords[1]);

  if (spectrum_.count(point))
    return spectrum_.at(point);
  else
    return 0;
}

std::unique_ptr<EntryList> Spectrum2D::_get_spectrum(std::initializer_list<Pair> list) {
  int min0, min1, max0, max1;
  if (list.size() != 2) {
    min0 = min1 = 0;
    max0 = max1 = metadata_.resolution;
  } else {
    Pair range0 = *list.begin(), range1 = *(list.begin()+1);
    min0 = range0.first; max0 = range0.second;
    min1 = range1.first; max1 = range1.second;
  }

  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  CustomTimer makelist(true);

  if (buffered_ && !temp_spectrum_.empty()) {
    for (auto it : temp_spectrum_) {
      int co0 = it.first.first, co1 = it.first.second;
      if ((min0 < co0) && (co0 < max0) && (min1 < co1) && (co1 < max1)) {
        Entry newentry;
        newentry.first.resize(2, 0);
        newentry.first[0] = co0;
        newentry.first[1] = co1;
        newentry.second = it.second;
        result->push_back(newentry);
      }
    }
  } else {
    for (auto it : spectrum_) {
      int co0 = it.first.first, co1 = it.first.second;
      if ((min0 < co0) && (co0 < max0) && (min1 < co1) && (co1 < max1)) {
        Entry newentry;
        newentry.first.resize(2, 0);
        newentry.first[0] = co0;
        newentry.first[1] = co1;
        newentry.second = it.second;
        result->push_back(newentry);
      }
    }
  }
  if (!temp_spectrum_.empty()) {
    boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
    while (!uniqueLock.try_lock())
      boost::this_thread::sleep_for(boost::chrono::seconds{1});
    temp_spectrum_.clear(); //assumption about client
  }
  PL_DBG << "Making list for " << metadata_.name << " took " << makelist.ms() << "ms filled with "
         << result->size() << " elements";
  return result;
}

void Spectrum2D::addEvent(const Event& newEvent) {
  uint64_t chan1_en = 0;
  uint64_t chan2_en = 0;
  if (newEvent.hit.count(pattern_[0]))
    chan1_en = newEvent.hit.at(pattern_[0]).energy >> shift_by_;
  if (newEvent.hit.count(pattern_[1]))
    chan1_en = newEvent.hit.at(pattern_[1]).energy >> shift_by_;
  spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)] += 1;
  if (buffered_)
    temp_spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)] =
        spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)];
  if (chan1_en > metadata_.max_chan) metadata_.max_chan = chan1_en;
  if (chan2_en > metadata_.max_chan) metadata_.max_chan = chan2_en;
  metadata_.total_count++;
}

bool Spectrum2D::_write_file(std::string dir, std::string format) const {
  if (format == "m") {
      write_m(dir + "/" + metadata_.name + ".m");
      return true;
  } else if (format == "esc") {
      write_esc(dir + "/" + metadata_.name + ".esc");
      return true;
  } else if (format == "spn") {
      write_spn(dir + "/" + metadata_.name + ".spn");
      return true;
  } else
      return false;
}

bool Spectrum2D::_read_file(std::string name, std::string format) {
  if (format == "esc")
    return read_esc(name);
  else if (format == "spn")
    return read_spn(name);
  else
    return false;
}

void Spectrum2D::write_m(std::string name) const {
  //matlab script
  std::ofstream myfile(name, std::ios::out | std::ios::app);
  myfile << "%MCA 2d spectrum from Pixie" << std::endl
         << "%  Bit precision: " << (16-shift_by_) << std::endl
         << "%  Channels     : " << metadata_.resolution << std::endl
         << "%  Total events : " << metadata_.total_count << std::endl
         << "clear;" << std::endl;
  for (auto it = spectrum_.begin(); it != spectrum_.end(); ++it)
    myfile << "coinc(" << (it->first.first + 1)
           << ", " << (it->first.second + 1)
           << ") = " << it->second << ";" << std::endl;

  myfile << "figure;" << std::endl
         << "imagesc(log(coinc));" << std::endl
         << "colormap(hot);" << std::endl;
  myfile.close();
}

void Spectrum2D::write_esc(std::string name) const {
  std::ofstream myfile(name, std::ios::out | std::ios::binary);

  float one;
  for (int i=0; i<2048; ++i) {
    for (int j=0; j<2048; ++j) {
      one = 0.0;
      std::pair<uint16_t,uint16_t> point(i, j);
      if (spectrum_.count(point))
        one = spectrum_.at(point);

      myfile.write((char*)&one, sizeof(float));

    }
  }
  myfile.close();
}

void Spectrum2D::write_spn(std::string name) const {
  std::ofstream myfile(name, std::ios::out | std::ios::binary);

  uint32_t one;
  for (int i=0; i<8192; ++i) {
    for (int j=0; j<8192; ++j) {
      one = 0;
      std::pair<uint16_t,uint16_t> point(i, j);
      if (spectrum_.count(point))
        one = static_cast<uint32_t>(spectrum_.at(point));

      myfile.write((char*)&one, sizeof(uint32_t));

    }
  }
  myfile.close();
}



bool Spectrum2D:: read_esc(std::string name) {
  //radware escl8r file
  std::ifstream myfile(name, std::ios::in | std::ios::binary);

  spectrum_.clear();
  metadata_.total_count = 0;
//  metadata_.max_chan = 0;
//  uint16_t max_i =0;

  float one;
  for (int i=0; i<2048; ++i) {
    // actually, i<2055, more junk there, not sure what it is
    for (int j=0; j<2048; ++j) {
      myfile.read ((char*)&one, sizeof(float));
      metadata_.total_count += one;
      if (one > 0)
        spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = one;
    }
  }
  metadata_.bits = 11;
  shift_by_ = 5;

  metadata_.detectors.resize(2);
  metadata_.detectors[0].name_ = "unknown1";
  metadata_.detectors[0].energy_calibrations_.add(Gamma::Calibration("Energy", metadata_.bits));
  metadata_.detectors[1].name_ = "unknown2";
  metadata_.detectors[1].energy_calibrations_.add(Gamma::Calibration("Energy", metadata_.bits));
  
  init_from_file(name);
}

bool Spectrum2D:: read_spn(std::string name) {
  //radware escl8r file
  std::ifstream myfile(name, std::ios::in | std::ios::binary);

  spectrum_.clear();
  metadata_.total_count = 0;
//  metadata_.max_chan = 0;
//  uint16_t max_i =0;

  uint32_t one;
  for (int i=0; i<8192; ++i) {
    for (int j=0; j<8192; ++j) {
      myfile.read ((char*)&one, sizeof(uint32_t));
      metadata_.total_count += one;
      if (one > 0)
        spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = one;
    }
  }
  metadata_.bits = 13;
  shift_by_ = 3;

  metadata_.detectors.resize(2);
  metadata_.detectors[0].name_ = "unknown1";
  metadata_.detectors[0].energy_calibrations_.add(Gamma::Calibration("Energy", metadata_.bits));
  metadata_.detectors[1].name_ = "unknown2";
  metadata_.detectors[1].energy_calibrations_.add(Gamma::Calibration("Energy", metadata_.bits));
  
  init_from_file(name);
}


void Spectrum2D::init_from_file(std::string filename) { 
  metadata_.match_pattern.resize(2, 0);
  metadata_.add_pattern.resize(2, 1);
  metadata_.add_pattern[0] = 1;
  metadata_.add_pattern[1] = 1;
  metadata_.name = boost::filesystem::path(filename).filename().string();
  std::replace( metadata_.name.begin(), metadata_.name.end(), '.', '_');
  metadata_.visible = true;
  metadata_.appearance = 4278190335;  //randomize?
  initialize();
  recalc_energies();
}


std::string Spectrum2D::_channels_to_xml() const {
  std::stringstream channeldata;

  int i=0, j=0;
  for (auto it = spectrum_.begin(); it != spectrum_.end(); ++it)
  {
    int this_i = it->first.first;
    int this_j = it->first.second;
    if (this_i > i) {
      channeldata << "+ " << (this_i - i) << " ";
      if (this_j > 0)
        channeldata << "0 " << this_j << " ";
      i = this_i;
      j = this_j + 1;
    }
    if (this_j > j)
      channeldata << "0 " << (this_j - j) << " ";
    channeldata << it->second  <<  " ";
    j = this_j + 1;
  }  
  return channeldata.str();
}

uint16_t Spectrum2D::_channels_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();
  metadata_.max_chan = 0;

  uint64_t i = 0, j = 0, max_i = 0;
  std::string numero, numero_z;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "+") {
      channeldata >> numero_z;
      if (j > metadata_.max_chan) metadata_.max_chan = j;
      i += boost::lexical_cast<uint64_t>(numero_z);
      j=0;
      if (j) max_i = i;
    } else if (numero == "0") {
      channeldata >> numero_z;
      j += boost::lexical_cast<uint64_t>(numero_z);
    } else {
      spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = boost::lexical_cast<uint64_t>(numero);
      j++;
    }
  }
  if (max_i > metadata_.max_chan)
    metadata_.max_chan = max_i;
  return metadata_.max_chan;
}

}}