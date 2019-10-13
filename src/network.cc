#include "network.hh"

#include <assert.h>
#include <iostream>

using namespace std;

MultiCastNetwork_Input::MultiCastNetwork_Input()
{
  rowWiseMC = new MultiCastController_InputNetwork[nRows_PE_Array];
  colWiseMC = new MultiCastController_InputNetwork*[nRows_PE_Array];
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    colWiseMC[i] = new MultiCastController_InputNetwork[nCols_PE_Array];
  }
}


MultiCastNetwork_Input::~MultiCastNetwork_Input()
{

  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    delete[] colWiseMC[i];
  }
  delete[] colWiseMC;
  delete[] rowWiseMC;
}


void MultiCastNetwork_Input::setupNetwork(bool *inConfig, bool *inForward,
  data_type *inData, uint8_t *inRowTag, uint8_t *inColTag,
  PE** PEArray, uint8_t networkId)
{
  // setup row-wise multicast controllers
  // connect them to the accelerator controller
  for(uint16_t i=0; i < nRows_PE_Array; i++) {

    rowWiseMC[i].setupController(1, i, 0);
    rowWiseMC[i].setConfig(inConfig);
    rowWiseMC[i].setForward(inForward);
    rowWiseMC[i].setInData(inData);
    rowWiseMC[i].setInTags(inColTag, inRowTag);
  }

  // set up column-wise multicast controllers
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    for(uint16_t j=0; j < nCols_PE_Array; j++) {
      // connect each column-wise multicast controller to its
      // corresponding row-wise multicast controller
      colWiseMC[i][j].setupController(0, i, j);
      colWiseMC[i][j].setConfig(rowWiseMC[i].getPtrConfigBus());
      colWiseMC[i][j].setForward(rowWiseMC[i].getPtrForwardBus());
      colWiseMC[i][j].setInData(rowWiseMC[i].getPtrOutData());
      colWiseMC[i][j].setInTags(rowWiseMC[i].getPtrColTagOut());

      // connect each column-wise multicast controller to a PE
      PEArray[i][j].setConfig(colWiseMC[i][j].getPtrConfigBus(), networkId);
      PEArray[i][j].setForward(colWiseMC[i][j].getPtrForwardBus(), networkId);
      PEArray[i][j].setInData(colWiseMC[i][j].getPtrOutData(), networkId);
    }
  }
}


void MultiCastNetwork_Input::execute()
{
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    for(uint16_t j=0; j < nCols_PE_Array; j++) {
      colWiseMC[i][j].execute();
    }
  }

  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    rowWiseMC[i].execute();
  }
}


void MultiCastController_InputNetwork::execute()
{
  if(!isRowWise_controller) {
    assert(((*config==0) || (*forward==0)) && "Column-wise controller cannot "
      "operate on data in both config and forward mode simultanously!");
  }

  // Do the follownig for the controller of an input/InOut network
  // if configuration of the controller is needed
  if(*config)
  {
    bus_forward_colWiseControllers = 0;
    counter_tag_match = 0;
    if((isRowWise_controller) && (*forward == 0) && (*rowTagIn == rowId) ||
       (!isRowWise_controller) && (*forward == 0) && (*colTagIn == colId))
    {
      // self-configure the controller
      if(counter_config == 0)
        configVal = *dataIn;
      else if(counter_config == 1)
        start_offset = *dataIn;
      else if (counter_config == 2)
        cnt_keepVal = *dataIn;
      else if (counter_config == 3)
        cnt_skipVal = *dataIn;
      counter_config++;
      bus_config_colWiseControllers = 0;

      // we don't need the following code in synthesized logic implementation
      // since counter_config should be of 2 bits
      if(counter_config == 4) counter_config = 0;
    }
    else if((isRowWise_controller) && (*forward) && (*rowTagIn == rowId))
    // forward the configuration to column-wise controllers
    {
      dataOut = *dataIn;
      colTagOut = *colTagIn;
      bus_config_colWiseControllers = 1;
    }
    else
      bus_config_colWiseControllers = 0;
  }
  // need to forward the data?
  else if((*config == 0) && (*forward == 1))
  {
    uint8_t inputTag = (isRowWise_controller) ? (*rowTagIn) : (*colTagIn);
    bus_config_colWiseControllers = 0;
    // After "offset" cycles, raise config value till "keepVal" Cycles
    // then, config value is 0 (i.e. no match) for "skipVal" cycles.
    // This repeats afterward till reconfiguration of controller
    // i.e., raise values for "keepVal" cycles, and skip for "skipVal" cycles.
    // When configVal is raised (during a total of "keepVal" cycles),
    // the configVal is compared with incoming tag of controller
    // If tag matches, data is forwarded further.
    if((counter_tag_match >= start_offset) &&
       (counter_tag_match < start_offset + cnt_keepVal) &&
       (configVal == inputTag))
    {
      dataOut = *dataIn;
      colTagOut = (isRowWise_controller) ? (*colTagIn) : 0;
      bus_forward_colWiseControllers = 1;
    }
    else
    {
      bus_forward_colWiseControllers = 0;
    }
    counter_tag_match++;
    if(counter_tag_match == start_offset + cnt_keepVal + cnt_skipVal)
      counter_tag_match = start_offset;
  }
  else
  {
    bus_config_colWiseControllers = 0;
    bus_forward_colWiseControllers = 0;
    counter_config = 0;
  }
}


void MultiCastController_InputNetwork::setupController(bool isRowWise,
    uint16_t row_idx, uint16_t col_idx)
{
  isRowWise_controller = isRowWise;
  rowId = row_idx;
  colId = col_idx;
}


void MultiCastController_InputNetwork::setConfig(bool *inConfig)
{
  config = inConfig;
}

void MultiCastController_InputNetwork::setForward(bool *inForward)
{
  forward = inForward;
}

void MultiCastController_InputNetwork::setInData(data_type *inData)
{
  dataIn = inData;
}

void MultiCastController_InputNetwork::setInTags(uint8_t *inColTag, uint8_t *inRowTag)
{
  colTagIn = inColTag;
  if(isRowWise_controller) rowTagIn = inRowTag;
}

bool* MultiCastController_InputNetwork::getPtrConfigBus()
{
  return &(this->bus_config_colWiseControllers);
}

bool* MultiCastController_InputNetwork::getPtrForwardBus()
{
  return &(this->bus_forward_colWiseControllers);
}

data_type* MultiCastController_InputNetwork::getPtrOutData()
{
  return &(this->dataOut);
}

uint8_t* MultiCastController_InputNetwork::getPtrColTagOut()
{
  return &(this->colTagOut);
}




MultiCastNetwork_Output::MultiCastNetwork_Output()
{
  rowWiseMC = new MultiCastController_OutputNetwork[nRows_PE_Array];
  colWiseMC = new MultiCastController_OutputNetwork*[nRows_PE_Array];
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    colWiseMC[i] = new MultiCastController_OutputNetwork[nCols_PE_Array];
  }
}


MultiCastNetwork_Output::~MultiCastNetwork_Output()
{

  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    delete[] colWiseMC[i];
  }
  delete[] colWiseMC;
  delete[] rowWiseMC;
}


void MultiCastNetwork_Output::setupNetwork(bool **outWriteBack,
  data_type **outData, PE** PEArray, uint8_t networkId)
{
  // setup row-wise multicast controllers
  // connect them to the accelerator controller
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    outWriteBack[i] = rowWiseMC[i].getPtrWriteBack();
    outData[i] = rowWiseMC[i].getPtrOutData();
    rowWiseMC[i].setupController(1);
  }

  // set up column-wise multicast controllers
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    for(uint16_t j=0; j < nCols_PE_Array; j++) {
      // connect each column-wise multicast controller to its
      // corresponding row-wise multicast controller
      rowWiseMC[i].setWriteBack(j, colWiseMC[i][j].getPtrWriteBack());
      rowWiseMC[i].setInData(j, colWiseMC[i][j].getPtrOutData());
      colWiseMC[i][j].setupController(0);

      // connect each column-wise multicast controller to a PE
      colWiseMC[i][j].setWriteBack(0, PEArray[i][j].getPtrWriteBack(networkId));
      colWiseMC[i][j].setInData(0, PEArray[i][j].getPtrOutData(networkId));
    }
  }
}


void MultiCastNetwork_Output::execute()
{
  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    rowWiseMC[i].execute(i, 0);
  }

  for(uint16_t i=0; i < nRows_PE_Array; i++) {
    for(uint16_t j=0; j < nCols_PE_Array; j++) {
      colWiseMC[i][j].execute(i, j);
    }
  }
}

void MultiCastController_OutputNetwork::execute(uint16_t rowId=0, uint16_t colId=0)
{
  uint16_t num_inputBus = (isRowWise_controller) ? nCols_PE_Array : 1;
  bool out_write_back = false;
  data_type out_data = 0;

  for(uint16_t it=0; it < num_inputBus; it++)
  {
    out_data     |= *(dataIn[it]);
    out_write_back |= *(writeBackIn[it]);
  }
  dataOut  = out_data;
  writeBackOut = out_write_back;
}


MultiCastController_OutputNetwork::MultiCastController_OutputNetwork()
{
  uint16_t num_inputBus = (isRowWise_controller) ? nCols_PE_Array : 1;
  dataIn = new data_type*[num_inputBus];
  writeBackIn = new bool*[num_inputBus];
}


MultiCastController_OutputNetwork::~MultiCastController_OutputNetwork()
{
  delete[] dataIn;
  delete[] writeBackIn;
}


void MultiCastController_OutputNetwork::setupController(bool isRowWise)
{
  isRowWise_controller = isRowWise;
}


bool* MultiCastController_OutputNetwork::getPtrWriteBack()
{
  return &(this->writeBackOut);
}

data_type* MultiCastController_OutputNetwork::getPtrOutData()
{
  return &(this->dataOut);
}


void MultiCastController_OutputNetwork::setInData(uint16_t idx, data_type *inData)
{
  dataIn[idx] = inData;
}


void MultiCastController_OutputNetwork::setWriteBack(uint16_t idx, bool *inWriteBack)
{
  writeBackIn[idx] = inWriteBack;
}
