#include <crab/support/debug.hpp>

#ifndef NCRABLOG
namespace crab {
bool CrabLogFlag = true; // todo: JR make false by default
std::set<std::string> CrabLog;

void CrabEnableLog(std::string x) {
  if (x.empty())
    return;
  CrabLogFlag = true;
  CrabLog.insert(x);
}
} // namespace crab

#else
namespace crab {
void CrabEnableLog(std::string x) {}
} // namespace crab
#endif

namespace crab {
unsigned CrabVerbosity = 5; // todo: JR make zero by default
void CrabEnableVerbosity(unsigned v) { CrabVerbosity = v; }

bool CrabWarningFlag = true;
void CrabEnableWarningMsg(bool v) { CrabWarningFlag = v; }

bool CrabSanityCheckFlag = false;
void CrabEnableSanityChecks(bool v) { CrabSanityCheckFlag = v; }

crab_os &get_msg_stream(bool timestamp) {
  crab::crab_os *result = &crab::outs();
  if (timestamp) {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%Y-%m-%d.%X] ", &tstruct);
    *result << buf;
  }
  return *result;
}
} // namespace crab
