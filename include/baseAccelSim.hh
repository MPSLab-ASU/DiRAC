#ifndef __BASEACCELSIM_HH__
#define __BASEACCELSIM_HH__

#include <string>
#include <iostream>

#include "controller.hh"
#include "definitions.hh"

uint16_t RF_passes = 0;
uint16_t SPM_passes = 0;

uint8_t total_operands = 0;
uint8_t output_operands = 0;
uint8_t input_operands = 0;
data_type **tensor_operands;
data_type **golden_outputs;
uint64_t *size_tensors;

vector<string> nameOfKernels;
vector<string> kernels;


/* Parameters for Architecture Specification */
uint32_t total_PEs = 0;
uint16_t nRows_PE_Array = 0;        // maximum allowed is 256
uint16_t nCols_PE_Array = 0;        // maximum allowed is 256
uint16_t RF_size = 0;               // Size of RF (in bytes) for each PE
uint32_t SPM_size;                  // Size of on-chip scratchpad memory
                                    // (in bytes), shared among PEs
uint16_t size_insMem = 0;           // Size of instruction memory in each PE
                                    // (size = instruction words)

uint16_t total_banks_in_SPM = 0;
uint16_t total_bytes_in_bank = 0; // (in bytes)

float accelerator_freq = 0;      // (in MHz)

// Latency model for data transfers via direct memory acceess (DMA)
float DMA_latency_setup = 0;
float DMA_latency_transfer_byte = 0;
float DMA_freq = 0;              // (in MHz)

// Energy Consumption Cost (technology specific, in pJ)
// Alternatively, can be supplied values normalized to MAC operation
// double EC0 = 0;  // energy cost for performing MAC operation
// double EC1 = 0;  // energy cost for accessing a register
// double EC2 = 0;  // energy cost for data communication via Multicast
// // Network-On-Chip (NoC) (same for inter-PE communication)
// double EC3 = 0;  // energy cost for accessing scratchpad memory
// // (i.e., global buffer shared among PEs)
// double EC4 = 0;  // energy cost for accessing DRAM

///
/// Register file of a PE is partitioned into multiple segments,
/// Currently all PEs share same configuration
/// Therefore, defining common configuration
/// Each variable indicates starting index for a data array in a buffer (total 2 buffers).
///
uint16_t *regs_buf1_startIdx_Op;
uint16_t *regs_buf2_startIdx_Op;

///
/// Iteration counts of the loops for accessing registers of each PE
///
uint16_t total_cycles_RF_pass = 0;
uint16_t total_compute_cycles_RF_pass = 0;
// After triggering PE, it delays write-back/reduction for some cycles
// (latency required to perform Write-to-RF from fetching instruction)
uint8_t  delay_PE_WB_reduce = 7;

uint8_t inNetworks = 3;
uint8_t outNetworks = 0;
uint8_t inOutNetworks = 1;
uint8_t totalNetworks = 4;
uint16_t *size_packetArr;
uint16_t *size_config_data_collect_write_back;
Packet **packetArr;   // Data packets for NoCs
Packet ***configArr;  // configuration packets for NoCs
// Offsets for data packets being collected via networks outputting data
uint16_t **config_data_collect_write_back;

uint32_t total_MC = 0;    // total Multicast Controllers

uint64_t tick = 0;
controller_state acc_controller_state = init;

uint8_t multiplier_stages = 2;
uint8_t adder_stages = 1;
uint8_t maxItems_PE_FIFO = 2;

std::vector<uint64_t> instArr;

uint32_t**  operand_start_addr_SPM_buffer;
bool**      read_operand_SPM_pass;
std::vector<uint32_t>**  read_operand_offset_base_addr_SPM;
std::vector<uint32_t>**  read_operand_burst_width_bytes_SPM_pass;
std::vector<uint32_t>**  read_operand_offset_base_addr_DRAM;
bool**      write_operand_SPM_pass;
std::vector<uint32_t>**  write_operand_offset_base_addr_SPM;
std::vector<uint32_t>**  write_operand_burst_width_bytes_SPM_pass;
std::vector<uint32_t>**  write_operand_offset_base_addr_DRAM;

bool**      read_data_RF_pass;
std::vector<uint8_t>**  RF_pass_network_read_operand;
std::vector<uint16_t>** RF_pass_network_read_operand_offset_network_packet;
std::vector<uint32_t>** RF_pass_network_read_operand_burst_width_elements;
std::vector<uint32_t>** RF_pass_network_read_operand_offset_base_addr_SPM;
bool**      write_data_RF_pass;
std::vector<uint8_t>**  RF_pass_network_write_operand;
std::vector<uint16_t>** RF_pass_network_write_operand_offset_network_packet;
std::vector<uint32_t>** RF_pass_network_write_operand_burst_width_elements;
std::vector<uint32_t>** RF_pass_network_write_operand_offset_base_addr_SPM;

uint16_t PE_trigger_latency = 0;  // size smaller than total registers.
bool**  trigger_exec_pass_PEs_exec;
bool**  trigger_exec_pass_zero_init;
bool**  trigger_exec_pass_writeback;
bool**  trigger_exec_pass_spatial_reduce;
bool**  trigger_exec_pass_update_buf_num_Op1;
bool**  trigger_exec_pass_update_buf_num_Op2;
bool**  trigger_exec_pass_update_buf_num_Op3;


// Manage indexing double buffering of scratchpad memory
// Separate indexing for prefetch and writeback
uint8_t* spm_buffer_number_op_prefetch;
uint8_t* spm_buffer_number_op_writeback;
uint8_t* spm_buffer_number_op_comp_read;
uint8_t* spm_buffer_number_op_comp_write;

// Required by only accelerator controller.
// Defining as global for debugging purposes.
uint16_t current_RF_pass = 0;
uint16_t current_SPM_pass = 0;

// Variables defining path of the input/data/configs files
// All are necessary to configure hardware execution.
std::string dir_test;
std::string dir_data;
std::string dir_arch;
std::string dir_kernel;
std::string configFileName;
std::string archSpecFileName;
std::string kernelFileName;
std::string instnsFileName;
std::string spmConfigFileName;
std::string dmacPrefetchManageFileName;
std::string dmacWBManageFileName;
std::string networkInfoFileName;
std::string interconnectConfigFileName;
std::string inputNetworkPacketsFileName;
std::string outputNetworkPacketsFileName;
std::string spmReadManageFileName;
std::string spmWriteManageFileName;
std::string configTriggerPEsFileName;

bool compare_golden = false;

class baseAccelSim {

public:
  baseAccelSim();

  virtual ~baseAccelSim();

  void read_inputs();
  void read_accel_configurations();
  void read_data();
  void write_data();
  void compare_golden_outputs();
  void free_memory();

private:
  void read_file_kernels(string kernelFileName);
  void set_kernel_parameters();
  void readArchSpec(string archSpecFileName);
  void readConfig(string configFileName);

  void read_data_operand(uint8_t op, string fileName);
  void write_data_operand(uint8_t op, string fileName);
  void read_golden_outputs();

  void read_file_instns(string fileName);
  void read_file_SPM_config(string spmConfigFileName);
  void read_file_DMAC_prefetch_management(string dmacPrefetchManageFileName);
  void read_file_DMAC_WB_management(string dmacWBManageFileName);
  void read_file_network_info(string networkInfoFileName);
  void read_file_config_interconnect(string interconnectConfigFileName);
  void read_file_input_network_packets(string inputNetworkPacketsFileName);
  void read_file_output_network_packets(string outputNetworkPacketsFileName);
  void read_file_SPM_read_management(string spmReadManageFileName);
  void read_file_SPM_write_management(string spmWriteManageFileName);
  void read_file_config_trigger_PEs(string configTriggerPEsFileName);
  void define_vars_SPM_management();
  void define_vars_network_comm_management();
};

#endif /* __BASEACCELSIM__HH__ */
