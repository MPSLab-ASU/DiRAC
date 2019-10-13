#include <iostream>
#include <memory>

#include "dma.hh"
#include "definitions.hh"
#include <cmath>

using namespace std;

DMA::DMA()
{
  latency_setup = DMA_latency_setup;
  latency_transfer_byte = DMA_latency_transfer_byte;
}

DMA::~DMA()
{
}


uint32_t DMA::calculate_data_transfer_delay(uint32_t count)
{
  float total_dma_latency = latency_setup + (latency_transfer_byte * count);
  float execution_cycles = ceil(total_dma_latency * (accelerator_freq / DMA_freq));
  return (uint32_t) execution_cycles;
}


void DMA::execute_tick()
{
  if (prefetch_in_progress)
  {
    if (count_prefetch < delay_prefetch)
    {
      // prefetch in progress
      ack_to_compute_device = false;
      count_prefetch += 1;
    }
    else if (count_prefetch == delay_prefetch)
    {
      // prefetch done
      ack_to_compute_device = true;
      count_prefetch = 0;
      prefetch_in_progress = false;
    }
    else
      assert("Should not reach here!");
  }
  else if(writeback_in_progress)
  {
    if (counter_write_back < delay_write_back)
    {
      ack_to_compute_device = false;
      counter_write_back += 1;
    }
    else if (counter_write_back == delay_write_back)
    {
      ack_to_compute_device = true;
      counter_write_back = 0;
      writeback_in_progress = false;
    }
    else
      assert("Should not reach here!");
  }
  else
    ack_to_compute_device = false;
}


void DMA::interface_host(bool *bus_req)
{
  req_from_compute_device = bus_req;
}

bool* DMA::get_ack_bus()
{
  return &(this->ack_to_compute_device);
}
