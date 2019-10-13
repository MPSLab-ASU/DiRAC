#include <iostream>
#include <memory>

#include "scratchpad.hh"

using namespace std;

SPM::SPM()
{
  scratchpad = new data_type[SPM_size];
}

SPM::~SPM()
{
  delete[] scratchpad;
}

data_type SPM::read(uint32_t idx)
{
  return scratchpad[idx];
}

void SPM::write(uint32_t idx, data_type data)
{
  scratchpad[idx] = data;
}

data_type* SPM::get_ptr()
{
  return this->scratchpad;
}
