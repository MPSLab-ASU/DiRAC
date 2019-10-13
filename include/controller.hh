#ifndef __CONTROLLER_HH__
#define __CONTROLLER_HH__

#include "pe.hh"
#include "network.hh"
#include "dma.hh"
#include "scratchpad.hh"

class controller
{
public:
  controller();
  virtual ~controller();

  void run(uint16_t **tensor_operands);
private:
  PE** PEs; //Pointer to array of PEs
  // multiple buses to trigger PEs individually
  // this can be used to give individual trigger word to each PE
  // which can inform it to do specific action.
  // otherwise, for a single trigger, a single bus would suffice.
  bool** bus_trigger_PEs_exec;
  bool** bus_trigger_PEs_zero_initialize;
  bool** bus_trigger_PEs_write_back;
  bool** bus_trigger_PEs_spatial_reduce;
  bool** bus_trigger_PEs_update_buf_num_Op1;
  bool** bus_trigger_PEs_update_buf_num_Op2;
  bool** bus_trigger_PEs_update_buf_num_Op3;

  bool needWriteBack;

  void advanceExecutionAtTick();
  void advanceTick();
  void reconfigure();


  void initialize();
  void configure_interconnect();
  void setup_interconnect();
  void setup_scratchpad();
  void setup_PEs();

  MultiCastNetwork_Input *network_inputs;
  bool *configNetwork;
  bool *forwardDataToNetwork;
  data_type *dataOutToNetwork;
  uint8_t *rowTagDataToNetwork;
  uint8_t *colTagDataToNetwork;
  uint16_t *cnt_data_distribution;

  MultiCastNetwork_Output *network_outputs;
  data_type ***dataInFromNetwork;
  bool ***writeBackFromNetwork;

  bool isFinished = 0;
  uint8_t terminate_count = 0;

  DMA *DMAC;
  SPM *scratchpad_mem;
  bool bus_req_DMA = false;
  bool *bus_ack_from_DMA;
  void setup_DMAC();
  void interface_DMAC(bool *bus_ack);

  uint8_t buffer_number_comp = 1;
  bool data_mov_SPM_DRAM_in_prog = false;
  uint8_t current_operand_prefetch = 0;
  uint8_t current_operand_writeback = 0;
  uint8_t current_DMAC_invocation_operand_prefetch = 0;
  uint8_t current_DMAC_invocation_operand_writeback = 0;

  bool PE_prefetch_RF_incomplete = true;
  bool PE_writeback_RF_incomplete = true;
  uint16_t counter_RF_pass_prefetch = 0;
  uint16_t counter_RF_pass_writeback = 0;
  uint8_t *cnt_prefetch_via_network_RF_pass;
  uint8_t *cnt_writeback_via_network_RF_pass;
  uint16_t *idx_read_network_packet;
  uint16_t *idx_write_network_packet;
  uint32_t cnt_bus_trigger = 0;
  bool* network_read_op_comm_completed;
  bool* network_write_op_comm_completed;
  bool* is_data_collected;
  data_type *collected_data;

  void execute_SPM_pass();
  void prefetch_to_SPM(uint16_t current_SPM_pass);
  void write_back_DRAM(uint16_t current_SPM_pass);
  void inc_SPM_pass();
  void execute_controller_pre_tick();

  void execute_RF_pass();
  void inc_RF_pass();
  void prefetch_to_PE(uint16_t current_RF_pass);
  void send_data_via_read_network(uint8_t operand, uint8_t network, uint16_t idx_packet, data_type data);
  void write_back_controller(uint16_t current_RF_pass);
  bool collect_data_via_write_network(uint8_t network, data_type* data);
  void trigger_PEs();
};

# endif /* __CONTROLLER__HH__ */
