#include <iostream>

#include "controller.hh"
#include "definitions.hh"

using namespace std;

controller::controller()
{
  setup_PEs();
  setup_interconnect();
  setup_DMAC();
  setup_scratchpad();
}


controller::~controller()
{
  for(auto i=0; i < nRows_PE_Array; i++) {
    delete[] PEs[i];
    delete[] bus_trigger_PEs_exec[i];
    delete[] bus_trigger_PEs_zero_initialize[i];
    delete[] bus_trigger_PEs_write_back[i];
    delete[] bus_trigger_PEs_spatial_reduce[i];
    delete[] bus_trigger_PEs_update_buf_num_Op1[i];
    delete[] bus_trigger_PEs_update_buf_num_Op2[i];
    delete[] bus_trigger_PEs_update_buf_num_Op3[i];

    delete[] dataInFromNetwork[i];
    delete[] writeBackFromNetwork[i];
  }
  delete[] PEs;
  delete[] bus_trigger_PEs_exec;
  delete[] bus_trigger_PEs_zero_initialize;
  delete[] bus_trigger_PEs_write_back;
  delete[] bus_trigger_PEs_spatial_reduce;
  delete[] bus_trigger_PEs_update_buf_num_Op1;
  delete[] bus_trigger_PEs_update_buf_num_Op2;
  delete[] bus_trigger_PEs_update_buf_num_Op3;

  delete[] network_inputs;
  delete[] network_outputs;
  delete[] configNetwork;
  delete[] forwardDataToNetwork;
  delete[] dataOutToNetwork;
  delete[] rowTagDataToNetwork;
  delete[] colTagDataToNetwork;
  delete[] cnt_data_distribution;
  delete[] dataInFromNetwork;
  delete[] writeBackFromNetwork;

  delete[] cnt_prefetch_via_network_RF_pass;
  delete[] idx_read_network_packet;
	delete[] cnt_writeback_via_network_RF_pass;
  delete[] idx_write_network_packet;
  delete[] network_read_op_comm_completed;
  delete[] network_write_op_comm_completed;

  delete[] is_data_collected;
  delete[] collected_data;

  delete DMAC;
  delete scratchpad_mem;
}


void controller::setup_PEs() {
	PEs = new PE*[nRows_PE_Array];
  bus_trigger_PEs_exec            = new bool*[nRows_PE_Array];
  bus_trigger_PEs_zero_initialize = new bool*[nRows_PE_Array];
  bus_trigger_PEs_write_back      = new bool*[nRows_PE_Array];
  bus_trigger_PEs_spatial_reduce  = new bool*[nRows_PE_Array];
  bus_trigger_PEs_update_buf_num_Op1 = new bool*[nRows_PE_Array];
  bus_trigger_PEs_update_buf_num_Op2 = new bool*[nRows_PE_Array];
  bus_trigger_PEs_update_buf_num_Op3 = new bool*[nRows_PE_Array];

	for(auto i=0; i < nRows_PE_Array; i++) {
		PEs[i] = new PE[nCols_PE_Array];
    bus_trigger_PEs_exec[i]             = new bool[nCols_PE_Array]();
    bus_trigger_PEs_zero_initialize[i]  = new bool[nCols_PE_Array]();
    bus_trigger_PEs_write_back[i]       = new bool[nCols_PE_Array]();
    bus_trigger_PEs_spatial_reduce[i]   = new bool[nCols_PE_Array]();
    bus_trigger_PEs_update_buf_num_Op1[i] = new bool[nCols_PE_Array]();
    bus_trigger_PEs_update_buf_num_Op2[i] = new bool[nCols_PE_Array]();
    bus_trigger_PEs_update_buf_num_Op3[i] = new bool[nCols_PE_Array]();

		for(auto j=0; j < nCols_PE_Array; j++) {
      PEs[i][j].setup(i,j);
			PEs[i][j].initialize();
      PEs[i][j].setTriggerFromController( &bus_trigger_PEs_exec[i][j],
        &bus_trigger_PEs_zero_initialize[i][j],
        &bus_trigger_PEs_write_back[i][j],
        &bus_trigger_PEs_spatial_reduce[i][j],
        &bus_trigger_PEs_update_buf_num_Op1[i][j],
        &bus_trigger_PEs_update_buf_num_Op2[i][j],
        &bus_trigger_PEs_update_buf_num_Op3[i][j]);
		}
	}
}


void controller::setup_interconnect()
{
  network_inputs        = new MultiCastNetwork_Input[inNetworks];
  network_outputs       = new MultiCastNetwork_Output[outNetworks];
  configNetwork         = new bool[inNetworks]();
  forwardDataToNetwork  = new bool[inNetworks]();
  dataOutToNetwork      = new data_type[inNetworks]();
  rowTagDataToNetwork   = new uint8_t[inNetworks]();
  colTagDataToNetwork   = new uint8_t[inNetworks]();
  cnt_data_distribution = new uint16_t[totalNetworks]();
  dataInFromNetwork     = new data_type**[outNetworks];
  writeBackFromNetwork  = new bool**[outNetworks];
  is_data_collected     = new bool[outNetworks]();
  collected_data        = new data_type[outNetworks]();

  for(uint16_t it=0; it < outNetworks; it++)
  {
    dataInFromNetwork[it]     = new data_type*[nRows_PE_Array];
    writeBackFromNetwork[it]  = new bool*[nRows_PE_Array];
  }

  for(uint8_t i=0; i < inNetworks; i++)
  {
    network_inputs[i].setupNetwork(configNetwork+i, forwardDataToNetwork+i,
      dataOutToNetwork+i, rowTagDataToNetwork+i, colTagDataToNetwork+i,
      PEs, i);
  }

  for(uint8_t i=0; i < outNetworks; i++)
  {
    network_outputs[i].setupNetwork(writeBackFromNetwork[i],
      dataInFromNetwork[i], PEs, i);
  }

  cnt_prefetch_via_network_RF_pass = new uint8_t[inNetworks]();
	cnt_writeback_via_network_RF_pass = new uint8_t[outNetworks]();
  idx_read_network_packet = new uint16_t[inNetworks]();
  idx_write_network_packet = new uint16_t[outNetworks]();
  network_read_op_comm_completed = new bool[inNetworks]();
  network_write_op_comm_completed = new bool[outNetworks]();
}


void controller::setup_DMAC()
{
  DMAC = new DMA();
  DMAC->interface_host(&bus_req_DMA);
  interface_DMAC(DMAC->get_ack_bus());
}


void controller::setup_scratchpad()
{
  scratchpad_mem = new SPM();
}


void controller::reconfigure()
{
  // One-time configuration of PEs
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    for(uint16_t j=0; j < nCols_PE_Array; j++) {
      PEs[i][j].setInsMem();
    }
  }

  current_SPM_pass = 0;
  current_RF_pass = 0;
}


void controller::run(data_type **tensor_operands)
{
  // reset configurations and counters
  reconfigure();

  while(!isFinished)
  {
    execute_controller_pre_tick();

    // execute networks for oututting data from PE array.
    // Controller reads data first, then output network should over-write
    // new data at the end of the tick.
    for(uint8_t i=0; i < outNetworks; i++) {
      network_outputs[i].execute();
    }

    // execute PEs
    for(uint16_t i=0; i < nRows_PE_Array; i++) {
  		for(uint16_t j=0; j < nCols_PE_Array; j++) {
  			PEs[i][j].executeAtTick();
  		}
  	}

    // execute networks for inputting data to PE array.
    // network reads data first, then controller should over-write
    // new data at the end of the tick.
    for(uint8_t i=0; i < inNetworks; i++) {
      network_inputs[i].execute();
    }

    // DMA controller advances its tick.
    DMAC->execute_tick();

    // execute controller logic
    advanceExecutionAtTick();

    // Update the global tick (cycle number).
    advanceTick();
  }
}


void controller::execute_SPM_pass()
{
  if(data_mov_SPM_DRAM_in_prog == 0)
  {
    if(current_SPM_pass < (SPM_passes + WB_delay_SPM_pass))
    {
      if ((current_SPM_pass >= SPM_passes) && (current_SPM_pass < SPM_passes + WB_delay_SPM_pass))
      {
        // don't do anything for read operands
        // process write operands
        if(current_operand_writeback < total_operands)
          write_back_DRAM(current_SPM_pass);
        else if(current_operand_writeback == total_operands)
          inc_SPM_pass();
        else
          assert("Should not reach here.");
      }
      else if (current_SPM_pass < WB_delay_SPM_pass)
      {
        // don't do anything for write operands
        // process read operands
        if(current_operand_prefetch < total_operands)
          prefetch_to_SPM(current_SPM_pass);
        else if(current_operand_prefetch == total_operands)
          inc_SPM_pass();
        else
          assert("Should not reach here.");
      }
      else
      {
        // process both read operands and write operands
        if(current_operand_prefetch < total_operands)
          prefetch_to_SPM(current_SPM_pass);
        else if (current_operand_writeback < total_operands)
          write_back_DRAM(current_SPM_pass);
        else if ((current_operand_prefetch == total_operands) && (current_operand_writeback == total_operands))
          inc_SPM_pass();
        else
          assert("Should not reach here.");
      }
    }
    else {
      acc_controller_state = terminate_exec;
      return;
    }
  }
}


void controller::advanceExecutionAtTick()
{
  if (acc_controller_state == init)
  {
    initialize();
  }
  else if (acc_controller_state == config_NoC)
  {
    configure_interconnect();
  }
  else if (acc_controller_state == exec_kernel)
  {
    execute_SPM_pass();
    execute_RF_pass();
  }
  else if(acc_controller_state == terminate_exec)
  {
    for(auto i=0; i < nRows_PE_Array; i++)
      for(auto j=0; j < nCols_PE_Array; j++)
        bus_trigger_PEs_exec[i][j] = 0;

    // acc_controller_state = terminate_exec;
    for(uint8_t i=0; i < inNetworks; i++)
    {
      configNetwork[i] = 0;
      forwardDataToNetwork[i] = 0;
      dataOutToNetwork[i] = 0;
    }
    isFinished = 1;
  }
  else
    assert("Undefined state for accelerator execution!");
}

void controller::advanceTick()
{
	tick++;
}


void controller::initialize()
{
  for(uint8_t i=0; i < totalNetworks; i++)
    cnt_data_distribution[i] = 0;
  acc_controller_state = config_NoC;

  for(uint8_t i=0; i < total_operands; i++) {
    spm_buffer_number_op_prefetch[i] = 1;
    spm_buffer_number_op_writeback[i] = 1;
    spm_buffer_number_op_comp_read[i] = 0;
    spm_buffer_number_op_comp_write[i] = 0;
  }

  PE_prefetch_RF_incomplete = true;
  PE_writeback_RF_incomplete = true;
}


void controller::configure_interconnect()
{
  for(uint8_t i=0; i < totalNetworks; i++)
  {
    configNetwork[i] = 1;
    // 4 is total elements in configArr packet
    uint32_t idxRow = cnt_data_distribution[i] / network_controller_config_params;
    uint16_t idxCol = cnt_data_distribution[i] % network_controller_config_params;
    if(cnt_data_distribution[i] >= nRows_PE_Array*network_controller_config_params)
      forwardDataToNetwork[i] = 1;
    else
      forwardDataToNetwork[i] = 0;

    rowTagDataToNetwork[i] = configArr[i][idxRow][idxCol].getRowTag();
    colTagDataToNetwork[i] = configArr[i][idxRow][idxCol].getColTag();
    dataOutToNetwork[i]    = configArr[i][idxRow][idxCol].getData();

    cnt_data_distribution[i]++;

    // all networks are configured simultanously
    // so controller does not need to sync-up or wait for a specific network
    if(cnt_data_distribution[i] == total_MC*network_controller_config_params)
    {
      cnt_data_distribution[i] = 0;
      acc_controller_state = exec_kernel;
    }
  }
}


void controller::prefetch_to_SPM(uint16_t current_SPM_pass)
{
  assert(current_operand_prefetch < total_operands && "invalid operand to be prefetched!");
  if(read_operand_SPM_pass[current_SPM_pass][current_operand_prefetch])
  {
    // schedule prefetch of operand to SPM
    if(current_DMAC_invocation_operand_prefetch < read_operand_offset_base_addr_SPM[current_SPM_pass][current_operand_prefetch].size())
    {
      uint8_t buffer_number_comm = spm_buffer_number_op_prefetch[current_operand_prefetch];
      uint32_t offset_SPM = read_operand_offset_base_addr_SPM[current_SPM_pass][current_operand_prefetch][current_DMAC_invocation_operand_prefetch];
      uint32_t offset_DRAM = read_operand_offset_base_addr_DRAM[current_SPM_pass][current_operand_prefetch][current_DMAC_invocation_operand_prefetch];
      uint32_t burst_size = read_operand_burst_width_bytes_SPM_pass[current_SPM_pass][current_operand_prefetch][current_DMAC_invocation_operand_prefetch];
      assert(burst_size > 0 && "burst size for DMAC request must be greater than zero.");
      data_type *address_operand_DRAM = tensor_operands[current_operand_prefetch] + (offset_DRAM / sizeof(data_type));
      data_type *address_operand_SPM = scratchpad_mem->get_ptr() + ((operand_start_addr_SPM_buffer[buffer_number_comm][current_operand_prefetch] + offset_SPM) / sizeof(data_type));
      bus_req_DMA = true;
      data_mov_SPM_DRAM_in_prog = true;
      DMAC->prefetch<data_type>(address_operand_DRAM, address_operand_SPM, burst_size);
      current_DMAC_invocation_operand_prefetch += 1;
    }
    else {
      current_DMAC_invocation_operand_prefetch = 0;
      current_operand_prefetch += 1;
    }
  }
  else
    current_operand_prefetch += 1;
}


void controller::write_back_DRAM(uint16_t current_SPM_pass)
{
  assert(current_operand_writeback < total_operands && "invalid operand for write back!");
  if(write_operand_SPM_pass[current_SPM_pass][current_operand_writeback])
  {
    // schedule write-back of operand from SPM to DRAM
    if(current_DMAC_invocation_operand_writeback < write_operand_offset_base_addr_SPM[current_SPM_pass][current_operand_writeback].size())
    {
      uint8_t buffer_number_comm = spm_buffer_number_op_writeback[current_operand_writeback];
      uint32_t offset_SPM = write_operand_offset_base_addr_SPM[current_SPM_pass][current_operand_writeback][current_DMAC_invocation_operand_writeback];
      uint32_t offset_DRAM = write_operand_offset_base_addr_DRAM[current_SPM_pass][current_operand_writeback][current_DMAC_invocation_operand_writeback];
      uint32_t burst_size = write_operand_burst_width_bytes_SPM_pass[current_SPM_pass][current_operand_writeback][current_DMAC_invocation_operand_writeback];
      assert(burst_size > 0 && "burst size for DMAC request must be greater than zero.");
      data_type *address_operand_DRAM = tensor_operands[current_operand_writeback] + (offset_DRAM / sizeof(data_type));
      data_type *address_operand_SPM = scratchpad_mem->get_ptr() + ((operand_start_addr_SPM_buffer[buffer_number_comm][current_operand_writeback] + offset_SPM) / sizeof(data_type));
      bus_req_DMA = true;
      data_mov_SPM_DRAM_in_prog = true;
      DMAC->write_back<data_type>(address_operand_SPM, address_operand_DRAM, burst_size);
      current_DMAC_invocation_operand_writeback += 1;
    }
    else {
      current_DMAC_invocation_operand_writeback = 0;
      current_operand_writeback += 1;
    }
  }
  else
    current_operand_writeback += 1;
}


void controller::inc_SPM_pass()
{
  // Move to next SPM pass only if PEs are done processing previous data
  // i.e., execution in all RF passes of an SPM pass.
  if ((current_RF_pass == RF_passes + WB_delay_RF_pass) || (current_SPM_pass == 0) || (current_SPM_pass == SPM_passes + 1))
  {
    current_SPM_pass += 1;
    current_operand_prefetch = 0;
    current_operand_writeback = 0;
    current_RF_pass = 0;

    for(uint8_t i=0; i < total_operands; i++)
    {
      bool update_idx_buffer_comm = false;
      bool update_idx_buffer_comp = false;

      // We need to update buffer idx responsible for communication in two cases:
      // 1) DMA is going to prefetch some data in the upcoming pass.
      // 2) DMA will write-back in the upcoming pass.
      // In either cases, need to communicate data between SPM and DRAM
      // via different buffer.
      if(current_SPM_pass < SPM_passes)
        if(read_operand_SPM_pass[current_SPM_pass][i])
          update_idx_buffer_comm = true;

      if((current_SPM_pass < SPM_passes + WB_delay_SPM_pass) && write_operand_SPM_pass[current_SPM_pass][i])
        update_idx_buffer_comm = true;

      if(update_idx_buffer_comm)
      {
        spm_buffer_number_op_prefetch[i] = (spm_buffer_number_op_prefetch[i] + 1) % 2;
        spm_buffer_number_op_writeback[i] = (spm_buffer_number_op_writeback[i] + 1) % 2;
      }

      // We need to update buffer idx responsible for computation in two cases:
      // 1) DMA prefetched some data in previous pass.
      //    Need to compute from different buffer.
      // 2) DMA will write-back in the upcoming pass.
      //    PE-array should write output in the different scratchpad buffer.

      if(current_SPM_pass > 0 && current_SPM_pass < SPM_passes + 1)
      {
        if(read_operand_SPM_pass[current_SPM_pass-1][i])
        {
          update_idx_buffer_comp = true;
        }
      }

      if((current_SPM_pass < SPM_passes + WB_delay_SPM_pass) && write_operand_SPM_pass[current_SPM_pass][i])
        update_idx_buffer_comp = true;

      if(update_idx_buffer_comp)
      {
        spm_buffer_number_op_comp_read[i] = (spm_buffer_number_op_comp_read[i] + 1) % 2;
        spm_buffer_number_op_comp_write[i] = (spm_buffer_number_op_comp_write[i] + 1) % 2;
      }
    }

  }
}


void controller::interface_DMAC(bool *bus_ack)
{
  bus_ack_from_DMA = bus_ack;
}


void controller::execute_controller_pre_tick()
{
  bus_req_DMA = false;

  // reset only if ack is received.
  data_mov_SPM_DRAM_in_prog = (*bus_ack_from_DMA == true) ? false : data_mov_SPM_DRAM_in_prog;

  for(uint8_t network=0; network < outNetworks; network++)
    is_data_collected[network] = collect_data_via_write_network(network, collected_data + network);
}


void controller::execute_RF_pass()
{
  // DMA prefetches data in 0th SPM pass.
  // Networks and PEs are already configured.
  // No need to configure networks further or communicate data.
  if(current_SPM_pass == 0)
  {
    for(uint8_t i=0; i < inNetworks; i++)
    {
      configNetwork[i] = 0;
      forwardDataToNetwork[i] = 0;
    }
  }

  if(current_SPM_pass > 0 && current_SPM_pass < SPM_passes + 1)
  {
    trigger_PEs();
    if ((current_RF_pass >= RF_passes) && (current_RF_pass < RF_passes + WB_delay_RF_pass))
    {
      // don't process read operands. need to process write operands
      if(PE_writeback_RF_incomplete)
        write_back_controller(current_RF_pass);
      else
        inc_RF_pass();
    }
    else if (current_RF_pass < WB_delay_RF_pass)
    {
      // don't process write operands. need to process read operands
      if(PE_prefetch_RF_incomplete)
        prefetch_to_PE(current_RF_pass);
      else
        inc_RF_pass();
    }
    else if((current_RF_pass >= WB_delay_RF_pass) && (current_RF_pass < RF_passes))
    {
      // process both read operands and write operands
      if(PE_prefetch_RF_incomplete)
        prefetch_to_PE(current_RF_pass);
      if (PE_writeback_RF_incomplete)
        write_back_controller(current_RF_pass);

      if ((PE_prefetch_RF_incomplete == 0) && (PE_writeback_RF_incomplete == 0))
        inc_RF_pass();
    }
  }
}


void controller::inc_RF_pass()
{
  current_RF_pass += 1;
  PE_prefetch_RF_incomplete = true;
  PE_writeback_RF_incomplete = true;
  counter_RF_pass_prefetch = 0;
  counter_RF_pass_writeback = 0;
  cnt_bus_trigger = 0;

  for(uint8_t i=0; i < inNetworks; i++)
  {
    configNetwork[i] = 0;
    forwardDataToNetwork[i] = 0;
  }
}


void controller::prefetch_to_PE(uint16_t current_RF_pass)
{
  assert(PE_prefetch_RF_incomplete == 1 && "Communication of data to PEs is incomplete.");
  bool prefetch_completed = true;

  for(uint8_t i=0; i < inNetworks; i++) {

    configNetwork[i] = 0;
    forwardDataToNetwork[i] = 0;
    network_read_op_comm_completed[i] = 1;

    if(read_data_RF_pass[current_RF_pass][i])
    {
      // schedule prefetch of operand to PE
      if(cnt_prefetch_via_network_RF_pass[i] < RF_pass_network_read_operand[current_RF_pass][i].size())
      {
        uint16_t burst_size_op = RF_pass_network_read_operand_burst_width_elements[current_RF_pass][i][cnt_prefetch_via_network_RF_pass[i]];
        network_read_op_comm_completed[i] = 0; // communication incomplete yet.
        if(idx_read_network_packet[i] < burst_size_op) {
          uint8_t operand = RF_pass_network_read_operand[current_RF_pass][i][cnt_prefetch_via_network_RF_pass[i]];
          uint8_t buffer_number_comp = spm_buffer_number_op_comp_read[operand];
          uint16_t idx_packet = RF_pass_network_read_operand_offset_network_packet[current_RF_pass][i][cnt_prefetch_via_network_RF_pass[i]];
          uint32_t offset_SPM = RF_pass_network_read_operand_offset_base_addr_SPM[current_RF_pass][i][cnt_prefetch_via_network_RF_pass[i]];
          uint32_t address_SPM = ((operand_start_addr_SPM_buffer[buffer_number_comp][operand] + offset_SPM) / sizeof(data_type)) + idx_read_network_packet[i];
          data_type data = scratchpad_mem->read(address_SPM);
          send_data_via_read_network(operand, i, idx_packet + idx_read_network_packet[i], data);
          idx_read_network_packet[i] += 1;
        }
        else {
          cnt_prefetch_via_network_RF_pass[i] += 1;
          idx_read_network_packet[i] = 0;
        }
      }
    }
    prefetch_completed &= (network_read_op_comm_completed[i] == 1);
  }

  counter_RF_pass_prefetch += 1;

  if ((counter_RF_pass_prefetch >= total_cycles_RF_pass) && (prefetch_completed)) {
    PE_prefetch_RF_incomplete = false;
    for(uint8_t i=0; i < inNetworks; i++)
      cnt_prefetch_via_network_RF_pass[i] = 0;
  }
}


void controller::send_data_via_read_network(uint8_t operand, uint8_t network, uint16_t idx_packet, data_type data)
{
  assert(idx_packet < size_packetArr[operand]);

  configNetwork[network] = 0;
  forwardDataToNetwork[network] = 1;

  rowTagDataToNetwork[network] = packetArr[operand][idx_packet].getColTag();
  colTagDataToNetwork[network] = packetArr[operand][idx_packet].getRowTag();
  dataOutToNetwork[network]    = data;
}


void controller::write_back_controller(uint16_t current_RF_pass)
{
  assert(PE_writeback_RF_incomplete == 1 && "Communication of data from PEs to controller is incomplete.");
  bool writeback_completed = true;

  for(uint8_t i=0; i < outNetworks; i++) {

    network_write_op_comm_completed[i] = 1;

    if(write_data_RF_pass[current_RF_pass][i])
    {
      // schedule writeback of operand to PE
      if(cnt_writeback_via_network_RF_pass[i] < RF_pass_network_write_operand[current_RF_pass][i].size())
      {
        uint16_t burst_size_op = RF_pass_network_write_operand_burst_width_elements[current_RF_pass][i][cnt_writeback_via_network_RF_pass[i]];
        network_write_op_comm_completed[i] = 0; // communication incomplete yet.
        if(idx_write_network_packet[i] < burst_size_op) {
          if(is_data_collected[i]) {
            uint8_t operand = RF_pass_network_write_operand[current_RF_pass][i][cnt_writeback_via_network_RF_pass[i]];
            uint8_t buffer_number_comp = spm_buffer_number_op_comp_write[operand];
            uint16_t offset_data_collect = config_data_collect_write_back[i][idx_write_network_packet[i]];
            uint32_t offset_SPM = RF_pass_network_write_operand_offset_base_addr_SPM[current_RF_pass][i][cnt_writeback_via_network_RF_pass[i]];
            uint32_t address_SPM = ((operand_start_addr_SPM_buffer[buffer_number_comp][operand] + offset_SPM) / sizeof(data_type)) + offset_data_collect;
            scratchpad_mem->write(address_SPM, collected_data[i]);
            idx_write_network_packet[i] += 1;
          }
        }
        else {
          cnt_writeback_via_network_RF_pass[i] += 1;
          idx_write_network_packet[i] = 0;
        }
      }
    }
    writeback_completed &= (network_write_op_comm_completed[i] == 1);
  }

  counter_RF_pass_writeback += 1;

  if ((counter_RF_pass_writeback >= total_cycles_RF_pass) && (writeback_completed)) {
    PE_writeback_RF_incomplete = false;
    for(uint8_t i=0; i < inNetworks; i++)
      cnt_writeback_via_network_RF_pass[i] = 0;
  }
}


bool controller::collect_data_via_write_network(uint8_t network, data_type* data)
{
  bool retVal = false;
  bool writeBack = false;
  data_type out_data = 0;
  for(uint16_t it=0; it < nRows_PE_Array; it++)
  {
    writeBack |= *(writeBackFromNetwork[network][it]);
    out_data |= *(dataInFromNetwork[network][it]);
  }
  if(writeBack)
  {
    *data = out_data;
    retVal = true;
  }

  return retVal;
}


void controller::trigger_PEs()
{
  // Trigger execution of PEs at the beginning of RF pass.
  if(cnt_bus_trigger == 0)
  {
    for(uint16_t rowID=0; rowID < nRows_PE_Array; rowID++) {
      for(uint16_t colID=0; colID < nCols_PE_Array; colID++) {
        bus_trigger_PEs_exec[rowID][colID]            = trigger_exec_pass_PEs_exec[current_SPM_pass][current_RF_pass];
        bus_trigger_PEs_zero_initialize[rowID][colID] = trigger_exec_pass_zero_init[current_SPM_pass][current_RF_pass];
        bus_trigger_PEs_update_buf_num_Op1[rowID][colID] = trigger_exec_pass_update_buf_num_Op1[current_SPM_pass][current_RF_pass];
        bus_trigger_PEs_update_buf_num_Op2[rowID][colID] = trigger_exec_pass_update_buf_num_Op2[current_SPM_pass][current_RF_pass];
        bus_trigger_PEs_update_buf_num_Op3[rowID][colID] = trigger_exec_pass_update_buf_num_Op3[current_SPM_pass][current_RF_pass];
      }
    }
  }
  else
  {
    for(uint16_t rowID=0; rowID < nRows_PE_Array; rowID++) {
      for(uint16_t colID=0; colID < nCols_PE_Array; colID++) {
        bus_trigger_PEs_exec[rowID][colID]            = 0;
        bus_trigger_PEs_zero_initialize[rowID][colID] = 0;
        bus_trigger_PEs_update_buf_num_Op1[rowID][colID] = 0;
        bus_trigger_PEs_update_buf_num_Op2[rowID][colID] = 0;
        bus_trigger_PEs_update_buf_num_Op3[rowID][colID] = 0;
      }
    }
  }


  assert(PE_trigger_latency > 0 && "PE trigger latency cannot be zero!");

  // Trigger write-back and spatial_reduce differently
  // PEs need to be triggered at certain offset-cycles for correct
  // communication via interconnect network.
  if((cnt_bus_trigger % PE_trigger_latency == 0) && (cnt_bus_trigger < PE_trigger_latency*total_PEs))
  {
    uint16_t rowID = (cnt_bus_trigger / (PE_trigger_latency * nCols_PE_Array)) % nRows_PE_Array;
    uint16_t colID = (cnt_bus_trigger / PE_trigger_latency) % nCols_PE_Array;
    assert(rowID < nRows_PE_Array && "Invalid rowID for a PE to be triggered.");
    assert(colID < nCols_PE_Array && "Invalid colID for a PE to be triggered.");
    bus_trigger_PEs_write_back[rowID][colID]      = trigger_exec_pass_writeback[current_SPM_pass][current_RF_pass];
    bus_trigger_PEs_spatial_reduce[rowID][colID]  = trigger_exec_pass_spatial_reduce[current_SPM_pass][current_RF_pass];
  }
  else {
    for(auto i=0; i < nRows_PE_Array; i++) {
      for(auto j=0; j < nCols_PE_Array; j++) {
        bus_trigger_PEs_write_back[i][j]      = 0;
        bus_trigger_PEs_spatial_reduce[i][j]  = 0;
      }
    }
  }
  if(cnt_bus_trigger < PE_trigger_latency*total_PEs)
    cnt_bus_trigger += 1;

}
