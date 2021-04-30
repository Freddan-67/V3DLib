#include "ProfileOutput.h"
#include "Support/basics.h"

std::string ProfileOutput::out_data::str() const {
  std::string ret;
  std::string params;
  params << "\"-n=" << num_qpus << " " << "-d=" << Dim << "\"";

  ret << tabbed(14, params) << ", " << timer;

  return ret;
}


ProfileOutput::ProfileOutput() {
  if (Platform::has_vc4()) {
    num_qpus = {1,4,8,12};
  } else {
    num_qpus = {1,8};
  }
}


std::string ProfileOutput::header() {
  std::string ret;

  ret << " - " << num_iterations << " iterations\n"
      << "Platform,         Params,     Time, Comments\n";

  return ret;
}


void ProfileOutput::add_compile(std::string const &label, Timer &timer, int Dim) {
  if (!ShowCompile) return; 

  std::string str;
  str << "\"compile " << label << "\"";

  output << out_data(str, timer.end(false), Dim, 0);
}


void ProfileOutput::add_call(std::string const &label, Timer &timer, int Dim, int num_qpus) {
  std::string str;
  str << "\"" << label << "\"";

  output << out_data(str, timer.end(false), Dim, num_qpus);
}


std::string ProfileOutput::dump() {
  std::string ret;

  std::string platform = (Platform::pi_version());
  std::string last_label;

  for (int i = 0; i < (int) output.size(); ++i) {
    auto const &item = output[i];

    std::string str;
    str << platform << "     , " << item.str() << ", ";

    if (last_label != item.label) {
      str << item.label;
      last_label = item.label; 
    }

    ret << str << "\n";
  }

  return ret;
}