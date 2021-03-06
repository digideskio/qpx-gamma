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
 *      FitykUtil -
 *
 ******************************************************************************/

#include "fityk_util.h"
#include <boost/lexical_cast.hpp>
#include "custom_logger.h"

std::string FitykUtil::var_def(std::string name,
                                      double val, double min, double max,
                                      int idx) {

  std::string ret = "$"  + name;
  if (idx >= 0)
    ret += boost::lexical_cast<std::string>(idx);
  ret += " = ~";
  ret += boost::lexical_cast<std::string>(val) +
         " [" + boost::lexical_cast<std::string>(min) +
          ":" + boost::lexical_cast<std::string>(max) + "]";
  return ret;
}

double FitykUtil::get_err(fityk::Fityk* f,
                          std::string funcname,
                          std::string varname)
{
  std::string command = "%" + funcname + "." + varname + ".error";
  DBG << "<FitykUtil> " << command;
  double ret = std::numeric_limits<double>::infinity();
  try {
    ret = f->calculate_expr(command);
  } catch (...) {
  }
  return ret;
}
