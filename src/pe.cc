#include <iostream>
#include <memory>

#include "pe.hh"
#include "definitions.hh"

using namespace std;

PE::PE()
{
  RegFile = new data_type[RF_size]();

  buf_in = new FIFO*[inNetworks];
  for(uint8_t i=0; i < inNetworks; i++)
    buf_in[i] = new FIFO;
  buf_out = new FIFO*[outNetworks];
  for(uint8_t i=0; i < outNetworks; i++)
    buf_out[i] = new FIFO;

  in_config   = new bool*[inNetworks];
  in_forward  = new bool*[inNetworks];
  in_data     = new data_type*[inNetworks];
  write_back  = new bool[outNetworks]();
  out_data    = new data_type[outNetworks]();

  // Note that zero initialization of following states is necessary.
  state_multiplier_stages = new bool[multiplier_stages]();
  state_adder_stages      = new bool[adder_stages]();

  temp_reg_multiply       = new data_type[multiplier_stages]();
  temp_reg_idx_write_Op3  = new uint16_t[multiplier_stages + adder_stages + 1]();
  temp_reg_add_Op1        = new data_type[adder_stages]();
  temp_reg_add_Op2        = new data_type[multiplier_stages + adder_stages - 1]();

  // Note that zero initialization of following signals is necessary.
  temp_sel_sig_adder_is_Op2_self = new bool[multiplier_stages + adder_stages]();
  temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass = new bool[2]();

  insMem = new uint64_t[size_insMem]();
}

PE::~PE()
{
  for(uint8_t i=0; i < inNetworks; i++)
    delete buf_in[i];
  for(uint8_t i=0; i < outNetworks; i++)
    delete buf_out[i];
  delete[] buf_in;
  delete[] buf_out;

  delete[] in_config;
  delete[] in_forward;
  delete[] in_data;

  delete[] write_back;
  delete[] out_data;

  delete[] RegFile;

  delete[] state_multiplier_stages;
  delete[] state_adder_stages;
  delete[] temp_reg_multiply;
  delete[] temp_reg_add_Op1;
  delete[] temp_reg_add_Op2;
  delete[] temp_reg_idx_write_Op3;
  delete[] temp_sel_sig_adder_is_Op2_self;
  delete[] temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass;

  delete[] insMem;
}

void PE::setup(uint16_t _row, uint16_t _col) {
  row = _row;
  col = _col;
  PE_State = idle;

  // ToDo: Configure dynamically
  // count_trigger_PE_exec += row*col + col;
  // count_trigger_write_back += row*col + col;
}

void PE::initialize()
{
  PE_State = idle;
  // ToDO: Decouple double-buffer management for netwro-FIFO interface vs.
  // that for PE processing data from RegFille.
  // In other words, two double-buffer indices each for communicate and compute
  // can allow managing variable interconnect networks vs. 3 operands of PE.
  regs_startIdx_Op1[0] = regs_buf1_startIdx_Op[0];
  regs_startIdx_Op2[0] = regs_buf1_startIdx_Op[1];
  regs_startIdx_Op3[0] = regs_buf1_startIdx_Op[2];
  regs_startIdx_Op1[1] = regs_buf2_startIdx_Op[0];
  regs_startIdx_Op2[1] = regs_buf2_startIdx_Op[1];
  regs_startIdx_Op3[1] = regs_buf2_startIdx_Op[2];
  buf_to_reg_idx_Op1 = regs_startIdx_Op1[0];
  buf_to_reg_idx_Op2 = regs_startIdx_Op2[0];
  buf_to_reg_idx_Op3 = regs_startIdx_Op3[0];
  reg_to_buf_idx_Op1 = regs_startIdx_Op3[0];
  // reg_read_Op1 = regs_startIdx_Op1;   // ifmaps
  // reg_read_Op2 = regs_startIdx_Op2; // weights
  // reg_read_Op3 = regs_startIdx_Op3;    // ofmap
  buffer_number_comm_op[0] = 0;
  buffer_number_comp_op[0] = 1;
  buffer_number_comm_op[1] = 0;
  buffer_number_comp_op[1] = 1;
  buffer_number_comm_op[2] = 0;
  buffer_number_comp_op[2] = 1;

  counter_PE_exec = 0;
  counter_PE_WB = 0;
  counter_writeback_Op1 = 0;
}

void PE::setTriggerFromController(bool *inBus_exec, bool *inBus_zero_initialize,
  bool *inBus_write_back, bool *inBus_spatial_reduction,
  bool *inBus_update_buf_Op1, bool *inBus_update_buf_Op2,
  bool *inBus_update_buf_Op3)
{
  trigger_from_controller_PE_exec             = inBus_exec;
  trigger_from_controller_zero_initialize     = inBus_zero_initialize;
  trigger_from_controller_write_back          = inBus_write_back;
  trigger_from_controller_spatial_reduce      = inBus_spatial_reduction;
  trigger_from_controller_update_buf_num_Op1  = inBus_update_buf_Op1;
  trigger_from_controller_update_buf_num_Op2  = inBus_update_buf_Op2;
  trigger_from_controller_update_buf_num_Op3  = inBus_update_buf_Op3;
}

///
/// happens in hardware
/// i.e. no instruction required to achieve this
///
void PE::readBuffers()
{
  if(!(buf_in[0]->isEmpty()))
  {
    data_type temp = buf_in[0]->pop();
    write_RF(buf_to_reg_idx_Op1, temp);
    buf_to_reg_idx_Op1++;
  }
  if(!(buf_in[1]->isEmpty()))
  {
    data_type temp = buf_in[1]->pop();
    write_RF(buf_to_reg_idx_Op2, temp);
    buf_to_reg_idx_Op2++;
  }
  if(inNetworks > 2)
  {
    if(!(buf_in[2]->isEmpty()))
    {
      data_type temp = buf_in[2]->pop();
      write_RF(buf_to_reg_idx_Op3, temp);
      buf_to_reg_idx_Op3++;
    }
  }
}

///
/// happens in hardware
/// i.e. no instruction required to achieve this
///
void PE::writeBuffers()
{
  for(uint8_t i=0; i < outNetworks; i++)
  {
    if(sig_write_back_current_RF_Pass && (counter_PE_WB >= delay_PE_WB_reduce))
    {
      if(counter_writeback_Op1 < cnt_output_Op1)
      {
        data_type temp = read_RF(reg_to_buf_idx_Op1);
        buf_out[i]->push(temp);
        // ToDO: Generalize following variables and counters to work with
        // i output networks; i > 1.
        reg_to_buf_idx_Op1++;
        counter_writeback_Op1++;
      }
    }
    else
      counter_PE_WB += 1;
  }
}

// if triggered then execute for next RF pass
// Upon getting trigger: exec_next_RF_pass = 1
//                       id_buf_reg_idx_Op[1-3] xor =  1

void PE::executeAtTick()
{
  // Pop data from output FIFOs to output networks
  execute_out_buf_read();

  // Push data from RF to output FIFO
  // Following condition (introducing delay slot) is needed only if PEs are
  // triggered for both execution and Write-Back at the same time.
  writeBuffers();

  if (PE_State == execute || state_write_RF)
  {
    execute_pipeline();
  }

  determine_next_state();

  // Populate RF from Input FIFOs if new data element(s) has arrived.
  readBuffers();

  // Pop data from input networks to input FIFOs
  execute_in_buf_write();
}

void PE::determine_next_state()
{
  // Execution events to occur when PE is in execution mode.
  if(PE_State == execute)
  {
    counter_PE_exec += 1;

    // If execution of an RF pass finishes, change current state as next state.
    if (counter_PE_exec == total_cycles_RF_pass)
    {
      PE_State = PE_Next_State;
      if(PE_Next_State == execute)
      {
        update_trigger_states();
        update_reg_idx_Ops();
      }
      else
      {
        state_fetch = false;
      }

      counter_PE_exec = 0;
    }
    // If compute cycles are less than total cycles for an RF pass, stall!
    else if((counter_PE_exec == total_compute_cycles_RF_pass) && (counter_PE_exec < total_cycles_RF_pass))
    {
      state_fetch = false;
    }
  }
  // When PE is idle, check whether it needs to execute.
  else if((PE_Next_State == execute) && (counter_PE_exec == 0))
  {
    update_trigger_states();
    update_reg_idx_Ops();
  }
  // PE waiting for an execution pass to perform write-back.
  else if((PE_WB_next_pass == true) && (counter_PE_exec == 0))
  {
    update_trigger_states();
    update_reg_idx_Ops();
  }
  // PE does not execute on new data, just writeback to interconnect.
  // else if((sig_write_back_current_RF_Pass == true) && (counter_PE_exec == 0))
  // {
  //   // do nothing
  // }

  // Since Write-Back is triggered separately than PE-execution or
  // prefetching data via network, we need to monitor states for write-back
  // continuously and update the states upon the trigger.
  update_trigger_states_write_data();

  // Bits/Word for trigger:
  // i) PE execution (Bit0),
  // ii) Write back (Bit1),
  // iii) Zero-Initialize current RF pass (Bit2)
  // iii) reduction (Bit3).
  PE_Next_State   = (*trigger_from_controller_PE_exec == 1) ? execute : PE_Next_State;
  PE_WB_next_pass = (*trigger_from_controller_write_back == 1) ? true : PE_WB_next_pass;
  PE_zero_initialize_next_pass  = (*trigger_from_controller_zero_initialize == 1) ? true : PE_zero_initialize_next_pass;
  PE_spatial_reduce_next_pass   = (*trigger_from_controller_spatial_reduce == 1) ? true : PE_spatial_reduce_next_pass;
  sig_update_buffer_num_Op1_next_pass = (*trigger_from_controller_update_buf_num_Op1 == 1) ? true : sig_update_buffer_num_Op1_next_pass;
  sig_update_buffer_num_Op2_next_pass = (*trigger_from_controller_update_buf_num_Op2 == 1) ? true : sig_update_buffer_num_Op2_next_pass;
  sig_update_buffer_num_Op3_next_pass = (*trigger_from_controller_update_buf_num_Op3 == 1) ? true : sig_update_buffer_num_Op3_next_pass;
}


void PE::execute_in_buf_write()
{
  for(uint8_t i=0; i < inNetworks; i++)
  {
    if(*in_forward[i])
    {
      buf_in[i]->push(*in_data[i]);
    }
  }
}

void PE::execute_out_buf_read()
{
  for(uint8_t i=0; i < outNetworks; i++)
  {
    if(!buf_out[i]->isEmpty())
    {
      (out_data[i]) = buf_out[i]->pop();
      (write_back[i]) = true;
    }
    else
    {
      (write_back[i]) = false;
      (out_data[i]) = 0;
    }
  }
}

data_type PE::read_RF(uint16_t idx)
{
  return RegFile[idx];
}

void PE::write_RF(uint16_t idx, data_type val)
{
  RegFile[idx] = val;
}


void PE::setConfig(bool *inConfig, uint8_t networkId)
{
  in_config[networkId] = inConfig;
}

void PE::setForward(bool *inForward, uint8_t networkId)
{
  in_forward[networkId] = inForward;
}

void PE::setInData(data_type *inData, uint8_t networkId)
{
  in_data[networkId] = inData;
}


data_type* PE::getPtrOutData(uint8_t networkId)
{
  return &(out_data[networkId]);
}

bool* PE::getPtrWriteBack(uint8_t networkId)
{
  return &(write_back[networkId]);
}

void PE::execute_pipeline()
{
  // write buffer
  if(state_write_RF == true)
    writeOutputReg();

  compute();

  if(state_read_RF == true)
    readInputRegs();

  if(state_decode == true)
    decode(fetched_instn);

  // Execution events to occur when PE is in execution mode.
  if(state_fetch == true)
    fetched_instn = insMem[counter_PE_exec];

  update_FFs();
  update_pipeline_states();
}

void PE::update_pipeline_states()
{
  state_write_RF = state_adder_stages[adder_stages-1];

  // Do not need following code when we have actual x-stage adder
  for(uint8_t it=adder_stages-1; it>0; it--)
    state_adder_stages[it] =  state_adder_stages[it-1];
  state_adder_stages[0] = state_multiplier_stages[multiplier_stages-1];

  // Do not need following code when we have actual x-stage multiplier
  for(uint8_t it=multiplier_stages-1; it>0; it--)
    state_multiplier_stages[it] =  state_multiplier_stages[it-1];
  state_multiplier_stages[0] = state_read_RF;

  state_read_RF = state_decode;
  state_decode = state_fetch;
}

void PE::compute_multiplication()
{
  // Do not need following code for temp_reg when we have actual x-stage multiplier
  for(uint8_t it=multiplier_stages-1; it>0; it--)
  {
      temp_reg_multiply[it] = temp_reg_multiply[it-1];
  }

  temp_reg_add_Op1[0] = temp_reg_multiply[multiplier_stages-1];

  // Do not need following condition when we have actual x-stage multiplier
  if(state_multiplier_stages[0])
  {
    temp_reg_multiply[0] = (in1 * in2);
  }
}


void PE::compute_addition()
{
  data_type add_Op1;
  data_type add_Op2;

  add_Op1 = temp_reg_add_Op1[adder_stages-1];

  if(temp_sel_sig_adder_is_Op2_self[multiplier_stages+adder_stages-1] == false) // (zero_initialize_Op_current_RF_Pass)
  // case 0:
  {
    add_Op2 = out_MAC;
  }
  else  // case 1:
  {
    add_Op2 = temp_reg_add_Op2[(multiplier_stages+adder_stages-1) - 1];
  }

  if(state_adder_stages[adder_stages-1])
    out_MAC = add_Op1 + add_Op2;

  // Update FFs for propagating input Operand2 for adder
  for(auto it1=(multiplier_stages+adder_stages-1) - 1; it1>0; it1--)
    temp_reg_add_Op2[it1] = temp_reg_add_Op2[it1-1];

  // pipelined in parallel with first stage of multiplier
  if(temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass[2] == 0)
  // case 0:
    temp_reg_add_Op2[0] = in3;
  else  // case 1:
    temp_reg_add_Op2[0] = 0;

  // Update FFs for propagating input Operand1 for adder
  for(auto it1=adder_stages-1; it1>0; it1--)
    temp_reg_add_Op1[it1] = temp_reg_add_Op1[it1-1];
}

void PE::update_FFs()
{
  // Update FlipFlops for copying signal for mux in first stage of multiplier,
  // which decides to select potential value (propagated in pipeline)
  // for operand2 of addition; initialization of accumulation is
  // selected between zero value vs. register read of Op3.
  // Signal is set before executing fetch stage (upon trigger for an RF pass),
  // and then signal gets propagated through fetch, decode, and RF read,
  // before it reaches to multiplier stage 0.
  for(uint8_t it=2; it>0; it--)
    temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass[it] = temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass[it-1];
  temp_sel_sig_adder_Op2_zero_initialize_current_RF_Pass[0] = sel_sig_adder_Op2_zero_initialize_current_RF_Pass;
}


// Execution of a pipelined X-stage multiplier and Y-stage adder
void PE::compute()
{
  compute_addition();
  compute_multiplication();
}

void PE::readInputRegs()
{
  in1 = read_RF(reg_read_Op1);
  in2 = read_RF(reg_read_Op2);
  in3 = read_RF(reg_read_Op3);
}

void PE::writeOutputReg()
{
  write_RF(temp_reg_idx_write_Op3[multiplier_stages+adder_stages], out_MAC);
}


void PE::setInsMem()
{
  assert(size_insMem >= instArr.size());
  for(uint16_t it = 0; it < instArr.size(); it++)
  {
    insMem[it] = instArr[it];
  }
}

void PE::update_reg_idx_Ops()
{
  // ToDO: Make the following parameterized per #networks for inputs/outputs
  buf_to_reg_idx_Op1 = regs_startIdx_Op1[buffer_number_comm_op[0]];
  buf_to_reg_idx_Op2 = regs_startIdx_Op2[buffer_number_comm_op[1]];
  buf_to_reg_idx_Op3 = regs_startIdx_Op3[buffer_number_comm_op[2]];
  reg_to_buf_idx_Op1 = regs_startIdx_Op3[buffer_number_comm_op[2]];
  reg_idx_Op1_comp = regs_startIdx_Op1[buffer_number_comp_op[0]];
  reg_idx_Op2_comp = regs_startIdx_Op2[buffer_number_comp_op[1]];
  reg_idx_Op3_comp = regs_startIdx_Op3[buffer_number_comp_op[2]];
}


void PE::update_trigger_states()
{
  if(PE_Next_State == execute)
  {
    PE_State = execute;
    state_fetch = true;
    PE_Next_State = idle;
  }

  sel_sig_adder_Op2_zero_initialize_current_RF_Pass = PE_zero_initialize_next_pass;
  PE_zero_initialize_next_pass = false;

  if(sig_update_buffer_num_Op1_next_pass)
  {
    buffer_number_comp_op[0] = (buffer_number_comp_op[0] + 1) % 2;
    buffer_number_comm_op[0] = (buffer_number_comm_op[0] + 1) % 2;
    sig_update_buffer_num_Op1_next_pass = false;
  }
  if(sig_update_buffer_num_Op2_next_pass)
  {
    buffer_number_comp_op[1] = (buffer_number_comp_op[1] + 1) % 2;
    buffer_number_comm_op[1] = (buffer_number_comm_op[1] + 1) % 2;
    sig_update_buffer_num_Op2_next_pass = false;
  }
  if(sig_update_buffer_num_Op3_next_pass)
  {
    buffer_number_comp_op[2] = (buffer_number_comp_op[2] + 1) % 2;
    buffer_number_comm_op[2] = (buffer_number_comm_op[2] + 1) % 2;
    sig_update_buffer_num_Op3_next_pass = false;
  }
}


void PE::update_trigger_states_write_data()
{
  if(PE_WB_next_pass == true)
  {
    sig_write_back_current_RF_Pass = PE_WB_next_pass;
    PE_WB_next_pass = false;
    counter_PE_WB = 0;
    counter_writeback_Op1 = 0;
  }
}


void PE::decode(uint64_t ins)
{
  // Update FlipFlops for copying signal for mux in last stage of adder,
  // which decides to select operand2 for addition, selected between
  // propagated value vs. self (data from the output latch of adder).
  for(uint8_t it=multiplier_stages+adder_stages-1; it>0; it--)
    temp_sel_sig_adder_is_Op2_self[it] = temp_sel_sig_adder_is_Op2_self[it-1];
  temp_sel_sig_adder_is_Op2_self[0] = sel_sig_adder_is_Op2_self;

  // Update FlipFlops for copying idx for writing back to same reg as read for Op3
  for(uint8_t it=multiplier_stages+adder_stages; it>0; it--)
    temp_reg_idx_write_Op3[it] = temp_reg_idx_write_Op3[it-1];
  temp_reg_idx_write_Op3[0] = reg_read_Op3;

  sel_sig_adder_is_Op2_self = (ins & 0x01);

  reg_read_Op3 = reg_idx_Op3_comp + ((ins >> 8) & 0xFF);
  reg_read_Op2 = reg_idx_Op2_comp + ((ins >> 16) & 0xFF);
  reg_read_Op1 = reg_idx_Op1_comp + ((ins >> 24) & 0xFF);
}
