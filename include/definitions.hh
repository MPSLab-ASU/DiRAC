#ifndef __DEFINITIONS_HH__
#define __DEFINITIONS_HH__

#include <inttypes.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include <stdlib.h> // For exit function
#include <iostream>


/**
  * Parameters for Convolution Layer from the Application
  **/
extern uint16_t RF_passes;
extern uint16_t SPM_passes;

typedef uint16_t data_type; // data_type common for all data operands;

extern uint8_t total_operands;
extern uint8_t output_operands;
extern uint8_t input_operands;
extern data_type **tensor_operands;
extern data_type **golden_outputs;
extern uint64_t *size_tensors;

/**
  * Parameters for Architecture Specification
  **/
extern uint32_t total_PEs;
extern uint16_t nRows_PE_Array;
extern uint16_t nCols_PE_Array;
extern uint16_t RF_size; // Size of RF (in bytes) for each PE
extern uint32_t SPM_size; // Size of on-chip scratchpad memory (in bytes), shared among PEs
extern uint16_t size_insMem; // Size of instruction memory in each PE.

extern uint16_t total_banks_in_SPM;
extern uint16_t total_bytes_in_bank;

extern float accelerator_freq;

// Latency model for data transfers via direct memory acceess (DMA)
extern float DMA_latency_setup;
extern float DMA_latency_transfer_byte;
extern float DMA_freq;

// // Energy Consumption Cost (technology specific, in pJ)
// // Alternatively, can be supplied values normalized to MAC operation
// extern double EC0;  // energy cost for performing MAC operation
// extern double EC1;  // energy cost for accessing a register
// extern double EC2;  // energy cost for data communication via Multicast
//                     // Network-On-Chip (NoC) (same for inter-PE communication)
// extern double EC3;  // energy cost for accessing scratchpad memory
//                     // (i.e., global buffer shared among PEs)
// extern double EC4;  // energy cost for accessing DRAM


///
/// Register file of a PE is partitioned into multiple segments,
/// Currently all PEs share same configuration
/// Therefore, defining common configuration
/// Starting index for a data array
///
extern uint16_t* regs_buf1_startIdx_Op;
extern uint16_t* regs_buf2_startIdx_Op;

///
/// Iteration counts of the loops for accessing registers of each PE
extern uint16_t total_cycles_RF_pass;
extern uint16_t total_compute_cycles_RF_pass;
extern uint8_t  delay_PE_WB_reduce;

extern uint8_t inNetworks;
extern uint8_t outNetworks;
extern uint8_t inOutNetworks;
extern uint8_t totalNetworks;
extern uint16_t *size_packetArr;
extern uint16_t *size_config_data_collect_write_back;
extern uint16_t **config_data_collect_write_back;

extern uint8_t multiplier_stages;

// Note that there is a possibility of recurrence for adder hardware, i.e.,
// data from the output latch can be required as input operand for next addition.
// So, total adder stages more than one are not recommended, unless
// code generation guarantees that it can avoid immediate accumulation of
// prior output.
extern uint8_t adder_stages;


extern uint32_t**  operand_start_addr_SPM_buffer;
extern bool**      read_operand_SPM_pass;
extern std::vector<uint32_t>**  read_operand_offset_base_addr_SPM;
extern std::vector<uint32_t>**  read_operand_burst_width_bytes_SPM_pass;
extern std::vector<uint32_t>**  read_operand_offset_base_addr_DRAM;
extern bool**      write_operand_SPM_pass;
extern std::vector<uint32_t>**  write_operand_offset_base_addr_SPM;
extern std::vector<uint32_t>**  write_operand_burst_width_bytes_SPM_pass;
extern std::vector<uint32_t>**  write_operand_offset_base_addr_DRAM;

extern bool**      read_data_RF_pass;
extern std::vector<uint8_t>**  RF_pass_network_read_operand;
extern std::vector<uint16_t>** RF_pass_network_read_operand_offset_network_packet;
extern std::vector<uint32_t>** RF_pass_network_read_operand_burst_width_elements;
extern std::vector<uint32_t>** RF_pass_network_read_operand_offset_base_addr_SPM;
extern bool**      write_data_RF_pass;
extern std::vector<uint8_t>**  RF_pass_network_write_operand;
extern std::vector<uint16_t>** RF_pass_network_write_operand_offset_network_packet;
extern std::vector<uint32_t>** RF_pass_network_write_operand_burst_width_elements;
extern std::vector<uint32_t>** RF_pass_network_write_operand_offset_base_addr_SPM;


extern uint16_t PE_trigger_latency;
extern bool**  trigger_exec_pass_PEs_exec;
extern bool**  trigger_exec_pass_zero_init;
extern bool**  trigger_exec_pass_writeback;
extern bool**  trigger_exec_pass_spatial_reduce;
extern bool**  trigger_exec_pass_update_buf_num_Op1;
extern bool**  trigger_exec_pass_update_buf_num_Op2;
extern bool**  trigger_exec_pass_update_buf_num_Op3;

extern uint8_t* spm_buffer_number_op_prefetch;
extern uint8_t* spm_buffer_number_op_writeback;
extern uint8_t* spm_buffer_number_op_comp_read;
extern uint8_t* spm_buffer_number_op_comp_write;


///
/// Parameters for Accelerator Controller
///
extern uint64_t tick;

enum controller_state {
    init=0,
    config_NoC,
    exec_kernel,
    terminate_exec
};

extern controller_state acc_controller_state;

#define row_tag_controller (1 << 15)

extern uint8_t maxItems_PE_FIFO;

// Array holding long instruction-word (common for all PEs)
extern std::vector<uint64_t> instArr;

// latency (in terms of SPM pass) for writing data back to DRAM
// Since data is read to SPM in an SPM pass x,
// data gets processed in {x+1}th pass, and written back in {x+2}nd pass.
#define WB_delay_SPM_pass 2
#define WB_delay_RF_pass 2

#define SPM_buffers 2


extern uint16_t current_RF_pass;
extern uint16_t current_SPM_pass;

// Paths of files for configuring the simulator and accelerator hardware
extern std::string dir_test;
extern std::string dir_data;
extern std::string dir_arch;
extern std::string dir_kernel;
extern std::string configFileName;
extern std::string archSpecFileName;
extern std::string kernelFileName;
extern std::string instnsFileName;
extern std::string spmConfigFileName;
extern std::string dmacPrefetchManageFileName;
extern std::string dmacWBManageFileName;
extern std::string networkInfoFileName;
extern std::string interconnectConfigFileName;
extern std::string inputNetworkPacketsFileName;
extern std::string outputNetworkPacketsFileName;
extern std::string spmReadManageFileName;
extern std::string spmWriteManageFileName;
extern std::string configTriggerPEsFileName;

extern bool compare_golden;

#endif /* DEFINITIONS_HH_ */
