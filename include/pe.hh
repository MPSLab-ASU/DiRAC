#ifndef __PE_HH__
#define __PE_HH__

#include <queue>
#include <deque>
#include "definitions.hh"

using namespace std;

///
/// Implementation of a FIFO with read and write pointer
/// if FIFO is full, data is over-written
///
class FIFO {
public:
  FIFO()
  {
    dataArr = new data_type[maxItems_PE_FIFO];
  }
  ~FIFO() { delete[] dataArr; }

  void push(data_type item) {
    dataArr[writePtr] = item;
    writePtr = (writePtr + 1) % maxItems_PE_FIFO;
  }

  data_type pop() {
    data_type item = dataArr[readPtr];
    readPtr = (readPtr + 1) % maxItems_PE_FIFO;
    return item;
  }

  bool isEmpty() { return (readPtr == writePtr); }
private:
  data_type *dataArr;
  uint8_t readPtr = 0;
  uint8_t writePtr = 0;
};


class PE {
public:

  PE();
  virtual ~PE();

  void setup(uint16_t _row, uint16_t _col);
  void initialize();
  void setTriggerFromController(bool *inBus_exec, bool *inBus_zero_initialize,
    bool *inBus_write_back, bool *inBus_spatial_reduction,
    bool *inBus_update_buf_Op1, bool *inBus_update_buf_Op2,
    bool *inBus_update_buf_Op3);
  void readBuffers();
  void writeBuffers();
  void determine_next_state();
  void executeAtTick();
  void execute_pipeline();
  void compute();

  /* Buffers */
  void execute_in_buf_write();
  void execute_out_buf_read();

  /* Register File */
  data_type read_RF(uint16_t idx);
  void write_RF(uint16_t idx, data_type val);
  void readInputRegs(); // For Inputs to FU
  void writeOutputReg(); // For Output from FU

  enum PEState {
    idle, execute
  };

  PEState PE_State = idle;
  PEState PE_Next_State = idle;

  void setConfig(bool *inConfig, uint8_t networkId);
  void setForward(bool *inForward, uint8_t networkId);
  void setInData(uint16_t *inData, uint8_t networkId);
  data_type * getPtrOutData(uint8_t networkId);
  bool* getPtrWriteBack(uint8_t networkId);

  void setInsMem();

private:
  uint16_t row;
  uint16_t col;

  /* Register File */
  data_type *RegFile;

  // Read/Write Index for Register File of a PE
  uint16_t reg_read_Op1 = 0;
  uint16_t reg_read_Op2 = 0;
  uint16_t reg_read_Op3 = 0;
  // uint16_t reg_write = 0;

  uint16_t buf_to_reg_idx_Op1 = 0;
  uint16_t buf_to_reg_idx_Op2 = 0;
  uint16_t buf_to_reg_idx_Op3 = 0;
  uint16_t reg_to_buf_idx_Op1 = 0;

  uint16_t reg_idx_Op1_comp = 0;
  uint16_t reg_idx_Op2_comp = 0;
  uint16_t reg_idx_Op3_comp = 0;
  uint16_t cnt_output_Op1 = 2;
  uint16_t counter_writeback_Op1 = 0;
  uint16_t counter_PE_WB = 0;

  /* Function Unit */
  data_type in1 = 0;
  data_type in2 = 0;
  data_type in3 = 0;
  bool skip_accumulate_pSum = 0;
  data_type out_MAC = 0;

  /* Buffers */
  FIFO      **buf_in;
  FIFO      **buf_out;
  bool      **in_config;
  bool      **in_forward;
  data_type  **in_data;
  bool      *write_back;
  data_type  *out_data;

  bool state_fetch = 0;
  bool state_decode = 0;
  bool state_read_RF = 0;
  bool *state_multiplier_stages;
  bool *state_adder_stages;
  bool state_write_RF = 0;

  bool compute_finished = 0;

  bool is_prev_psum_zero = 1;

  uint16_t counter_PE_exec = 0;
  // Bits/Word for trigger:
  // i) PE execution (Bit0),
  // ii) Write back (Bit1),
  // iii) reduction (Bit2).
  bool *trigger_from_controller_PE_exec;
  bool *trigger_from_controller_zero_initialize;
  bool *trigger_from_controller_write_back;
  bool *trigger_from_controller_spatial_reduce;
  bool *trigger_from_controller_update_buf_num_Op1;
  bool *trigger_from_controller_update_buf_num_Op2;
  bool *trigger_from_controller_update_buf_num_Op3;

  bool PE_WB_next_pass;
  bool PE_zero_initialize_next_pass;
  bool PE_spatial_reduce_next_pass;
  bool sig_update_buffer_num_Op1_next_pass;
  bool sig_update_buffer_num_Op2_next_pass;
  bool sig_update_buffer_num_Op3_next_pass;

  uint8_t buffer_number_comm_op[3];
  uint8_t buffer_number_comp_op[3];
  uint64_t* insMem;
  uint64_t fetched_instn;

  uint16_t regs_startIdx_Op1[2];
  uint16_t regs_startIdx_Op2[2];
  uint16_t regs_startIdx_Op3[2];

  data_type* temp_reg_multiply;
  data_type* temp_reg_add_Op1;
  data_type* temp_reg_add_Op2;
  uint16_t* temp_reg_idx_write_Op3;

  bool sel_sig_adder_Op2_zero_initialize_current_RF_Pass = 0;
  bool *temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass;
  bool sig_write_back_current_RF_Pass = 0;

  bool sel_sig_adder_is_Op2_self = 0;
  // Propagate sel_sig_adder_is_Op2_self (obtained in decode) through RF_Read,
  // multiplier_stages, and adder_stages-1 pipeline stages.
  bool* temp_sel_sig_adder_is_Op2_self;

  void decode(uint64_t ins);
  void update_reg_idx_Ops();
  void update_trigger_states();
  void update_trigger_states_write_data();
  void update_pipeline_states();
  void update_FFs();
  void compute_multiplication();
  void compute_addition();
};

#endif
