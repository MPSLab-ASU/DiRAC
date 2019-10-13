#ifndef __SCRATCHPAD_HH__
#define __SCRATCHPAD_HH__

#include "definitions.hh"

using namespace std;

///
/// Implementation of an on-chip scratchpad memory (shared by PE-array)
///
class SPM {
public:
  SPM();
  ~SPM();

  data_type read(uint32_t idx);
  void write(uint32_t idx, data_type data);
  data_type* get_ptr();
private:
  data_type* scratchpad;
};


#endif
