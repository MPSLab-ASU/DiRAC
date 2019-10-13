#ifndef __NETWORK_HH__
#define __NETWORK_HH__

#include "definitions.hh"
#include "pe.hh"

class MultiCastController_InputNetwork {
public:
  MultiCastController_InputNetwork() {};
  ~MultiCastController_InputNetwork() {};

  void setupController(bool isRowWise, uint16_t row_idx, uint16_t col_idx);
  void setConfig(bool *inConfig);
  void setForward(bool *inForward);
  void setInData(data_type *inData);
  void setInTags(uint8_t *inColTag, uint8_t *inRowTag=nullptr);

  bool* getPtrConfigBus();
  bool* getPtrForwardBus();
  data_type* getPtrOutData();
  uint8_t* getPtrColTagOut();

  void execute();
private:
  // I/O signals
  bool      *config;
  bool      *forward;
  data_type  *dataIn;
  data_type  dataOut = 0;
  bool      bus_config_colWiseControllers = 0;
  bool      bus_forward_colWiseControllers = 0;

  uint8_t   *rowTagIn;
  uint8_t   *colTagIn;
  uint8_t   colTagOut = 0;

  // properties of controller
  // dynamically configurable
  uint8_t configVal     = 0;
  uint8_t start_offset  = 0;
  uint8_t cnt_keepVal   = 0;
  uint8_t cnt_skipVal   = 0;

  // hard-programmed properties
  // set once when accelerator is initialized
  bool isRowWise_controller = 0;
  uint16_t rowId = 0; // used for configuration only if row-wise controller
  uint16_t colId = 0; // used for configuration only if column-wise controller

  // counters for internal management
  uint8_t counter_config = 0; // 2-bit counter
  uint16_t counter_tag_match = 0;
};

class MultiCastController_OutputNetwork {
public:
  MultiCastController_OutputNetwork();
  ~MultiCastController_OutputNetwork();

  void setupController(bool isRowWise);
  void setInData(uint16_t idx, data_type *inData);
  void setWriteBack(uint16_t idx, bool *inWriteBack);

  bool* getPtrWriteBack();
  data_type* getPtrOutData();

  void execute(uint16_t rowId, uint16_t colId);
private:
  // I/O signals
  // Input is coming from PEArraySide, multiple PEs/controllers can drive it.
  // Output is going to AccelControllerSide, one MC-controller or
  // acc-controller will read it.
  // ToDo:  Reverse the direction of buses, i.e., inputs are read through
  // pointers and  outputs are written through variables.
  bool      **writeBackIn;
  data_type **dataIn;
  bool      writeBackOut;
  data_type dataOut;

  // hard-programmed properties
  // set once when accelerator is initialized
  bool isRowWise_controller = 0;
};


class MultiCastNetwork_Input {
public:
  MultiCastNetwork_Input();
  ~MultiCastNetwork_Input();
  void setupNetwork(bool *inConfig, bool *inForward,
    data_type *inData, uint8_t *inRowTag, uint8_t *inColTag,
    PE** PEArray, uint8_t networkId);
  void execute();
private:
  MultiCastController_InputNetwork *rowWiseMC;
  MultiCastController_InputNetwork **colWiseMC;
};


class MultiCastNetwork_Output {
public:
  MultiCastNetwork_Output();
  ~MultiCastNetwork_Output();
  void setupNetwork(bool **outWriteBack,
    data_type **outData, PE** PEArray, uint8_t networkId);
  void execute();
private:
  MultiCastController_OutputNetwork *rowWiseMC;
  MultiCastController_OutputNetwork **colWiseMC;
};


#define network_controller_config_params 4

class Packet {
public:
  Packet()
  {
    rowTag = 0;
    colTag = 0;
    data = 0;
  }
  ~Packet() {};
  uint16_t getData() { return data; }
  uint8_t getRowTag() { return rowTag; }
  uint8_t getColTag() { return colTag; }
  void setData(uint16_t inData) { data = inData; };
  void setRowTag(uint8_t row) { rowTag = row; };
  void setColTag(uint8_t col) { colTag = col; };
private:
  uint8_t rowTag;
  uint8_t colTag;
  data_type data;
};

// packetArr0 or configArr0 or NoC0 is for Op1 in PEs e.g., input feature map data
// packetArr1 or configArr1 or NoC1 is for Op2 in PEs e.g., weights
// packetArr2 or configArr2 or NoC2 is for Op3 in PEs e.g., psum_in
// packetArr3 or configArr3 or NoC3 is for Op3 in PEs e.g., psum_out
extern Packet **packetArr;  // data packets for NoCs
extern Packet ***configArr; // configuration packets for NoCs
extern uint32_t total_MC;   // total Multicast Controllers in each MC network

#endif /* __NETWORK__HH__ */
