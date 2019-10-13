#ifndef __DMA_HH__
#define __DMA_HH__

#include <assert.h>
#include <cstring>

using namespace std;

///
/// Implementation of a direct-memory access (DMA) controller
///
class DMA {
public:
  DMA();
  ~DMA();

  // prefetch data from DRAM
  template <typename Type> void prefetch(Type *ptr_src, Type *ptr_dest, uint32_t count)
  {
    assert((*req_from_compute_device == true) && "Req signal from compute device is not generated.");
    assert((prefetch_in_progress == false) && "DMA cannot handle more than one prefetch request.");
    assert((writeback_in_progress == false) && "DMA cannot handle more than one read/write request.");

    uint64_t total_bytes = count;
    memcpy(ptr_dest, ptr_src, total_bytes);

    delay_prefetch = calculate_data_transfer_delay(total_bytes);
    prefetch_in_progress = true;
  }


  // write-back data to DRAM
  template <typename Type> void write_back(Type *ptr_src, Type *ptr_dest, uint32_t count)
  {
    assert((*req_from_compute_device == true) && "Req signal from compute device is not generated.");
    assert((prefetch_in_progress == false) && "DMA cannot handle more than one read/write request.");
    assert((writeback_in_progress == false) && "DMA cannot handle more than one read/write request.");

    uint64_t total_bytes = count;
    memcpy(ptr_dest, ptr_src, total_bytes);

    delay_write_back = calculate_data_transfer_delay(total_bytes);
    writeback_in_progress = true;
  }

  void execute_tick();
  void interface_host(bool *bus_req);
  bool* get_ack_bus();

private:
  float latency_setup = 0;
  float latency_transfer_byte = 0;

  bool* req_from_compute_device;
  bool ack_to_compute_device = false;
  bool prefetch_in_progress = false;
  bool writeback_in_progress = false;

  uint32_t count_prefetch = 0;
  uint32_t counter_write_back = 0;
  uint32_t delay_prefetch = 0;
  uint32_t delay_write_back = 0;

  uint32_t calculate_data_transfer_delay(uint32_t count);

};

#endif
