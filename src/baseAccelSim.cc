#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include "parse.hh"
#include "baseAccelSim.hh"


baseAccelSim::baseAccelSim()
{
}

baseAccelSim::~baseAccelSim()
{
}


void baseAccelSim::read_inputs()
{
	read_file_kernels(kernelFileName);
	set_kernel_parameters();
	readArchSpec(archSpecFileName);
	readConfig(configFileName);
}


void baseAccelSim::read_file_kernels(string kernelFileName) {
	string key, value;
	uint16_t it=0;

	ifstream inFile(kernelFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << kernelFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		tokenStream >> key >> value;
		kernels.push_back(value);
		nameOfKernels.push_back(key);
		it++;
	}
	inFile.close();

	assert(kernels.size() > 0 && "No bytes of the kernels in the file ");
}


void baseAccelSim::readArchSpec(string archSpecFileName) {
	string key, value, line;
	map <string, string> archSpec;

	ifstream inFile(archSpecFileName.c_str());

	if(!inFile)
	{
		cout << "Cannot open and read file: " << archSpecFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	while(getline(inFile, line)) {
		istringstream tokenStream(line);
		tokenStream >> key >> value;
		// Skip comments beginning with #
		// Skip empty lines
		if ( (key.find("#") != std::string::npos) || (line.empty()) )
	    continue;
		archSpec[key] = value;
	}
	inFile.close();

	nRows_PE_Array 				= stoi(archSpec["nRows_PE_Array"]);
	nCols_PE_Array 				= stoi(archSpec["nCols_PE_Array"]);
	RF_size 							= stoi(archSpec["RF_size"]);
	SPM_size 							= stoi(archSpec["SPM_size"]);
	total_banks_in_SPM 		= stoi(archSpec["total_banks_in_SPM"]);
	total_bytes_in_bank = stoi(archSpec["total_bytes_in_bank"]);
	accelerator_freq 			= stoi(archSpec["accelerator_freq"]);
	DMA_latency_setup 		= stoi(archSpec["DMA_init_cycles"]);
	DMA_latency_transfer_byte		= stod(archSpec["DMA_cycles_per_byte"]);
	DMA_freq 							= stoi(archSpec["DMA_freq"]);
	multiplier_stages			= stoi(archSpec["multiplier_stages"]);
	adder_stages 					= stoi(archSpec["adder_stages"]);
	maxItems_PE_FIFO			= stoi(archSpec["maxItems_PE_FIFO"]);
	size_insMem						= stoi(archSpec["size_insMem"]);

	inNetworks						= stoi(archSpec["inNetworks"]);
	outNetworks						= stoi(archSpec["outNetworks"]);

	total_PEs 						= nRows_PE_Array * nCols_PE_Array;
	// Consider total number of registers in the Register Files of PEs
	RF_size 							= RF_size / sizeof(data_type);
	totalNetworks					= inNetworks + outNetworks;
	// PE pipeline stages: Fetch+ Decoder + RF_Read + Multiplier+Adder + RF_Write
	uint8_t PE_pipeline_stages	= multiplier_stages + adder_stages + 4;
	delay_PE_WB_reduce		= PE_pipeline_stages;

	assert(nRows_PE_Array >= 1 && "Total PEs in a row should be 1 or more.");
	assert(nCols_PE_Array >= 1 && "Total PEs in a column should be 1 or more.");
	assert(RF_size >= 1 && "Registers in the RFs must be 1 or more.");
	assert(SPM_size >= RF_size && "Size of scratchpad memory must be larger than RF size.");
	assert(total_banks_in_SPM >= 1 &&
		"Total banks in Scratch-pad should be 1 or more.");
	assert(total_bytes_in_bank >= 1 &&
		"Total bytes in a bank of the Scratch-pad Memory must be 1 or more.");
	assert(SPM_size == (total_banks_in_SPM * total_bytes_in_bank) &&
		"SPM size must match SPM configuration with multi-banks.");
	assert(accelerator_freq > 0 &&
		"Frequency of the dataflow accelerator must be larger than 0 Hz.");
	assert(DMA_latency_setup > 0 &&
		"Initiation Cycles for DMA invocation should be greater than 0.");
	assert(DMA_latency_transfer_byte > 0 &&
		"Cycles per byte for transfering data through DMA must be greater than 0.");
	assert(DMA_freq > 0 &&
		"Frequency of the DMA for transferring the data must be larger than 0 Hz.");
	assert(multiplier_stages > 0 &&
		"Multiplier stages of Multiply-and-ACcumulate unit must be greater than 0.");
	assert(adder_stages > 0 &&
		"Adder stages of Multiply-and-ACcumulate unit must be greater than 0.");
	assert(maxItems_PE_FIFO > 1 && "Length of FIFOs in PEs for communicating via interconnect should be two or more.");
	assert(size_insMem > 0 && "Total instructions for executing an RF pass should be one or more.");
	assert(inNetworks >= 2 && "Total networks to communicate input data should be 2 or more.");
	assert(outNetworks >= 1 && "Total networks to communicate output data should be 1 or more.");
}

void baseAccelSim::readConfig(string configFileName) {
	string key, value, line;
	map <string, string> configs;

	ifstream inFile(configFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << configFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	while(getline(inFile, line)) {
		istringstream tokenStream(line);
		tokenStream >> key >> value;
		// Skip comments beginning with #
		// Skip empty lines
		if ( (key.find("#") != std::string::npos) || (line.empty()) )
			continue;
		configs[key] = value;
	}
	inFile.close();

	RF_passes 			= stoi(configs["RF_passes_in_SPM_pass"]);
	SPM_passes 			= stoi(configs["total_SPM_pass"]);
	total_cycles_RF_pass 		= stoi(configs["total_cycles_RF_pass"]);
	total_compute_cycles_RF_pass = stoi(configs["total_compute_cycles_RF_pass"]);

	regs_buf1_startIdx_Op = new uint16_t[total_operands]();
	regs_buf2_startIdx_Op = new uint16_t[total_operands]();
	uint8_t count_nonZero_reg_startIdx = 0;

	for(uint8_t i=0; i < total_operands; i++)
	{
		regs_buf1_startIdx_Op[i] = stoi(configs["regs_buf1_startIdx_Op"+ std::to_string(i+1)]);
		regs_buf2_startIdx_Op[i] = stoi(configs["regs_buf2_startIdx_Op"+ std::to_string(i+1)]);

		if(regs_buf1_startIdx_Op[i] > 0) count_nonZero_reg_startIdx++;
	}
	// no need to do for buf2; double buffering of RF may not be needed.
	// We need to work with at least two tensors: one for input, one for output.
	// So, at least one tensor out of the two should start at non-zero reg. idx.
	assert(count_nonZero_reg_startIdx >= 1 && "Incorrect startIdx for RegFile");

	assert(RF_passes >= 1 && "Total RF passes must be 1 or more for a kernel execution.");
	assert(SPM_passes >= 1 && "Total SPM passes must be 1 or more for a kernel execution.");
	assert(total_cycles_RF_pass > 0 && "Total cycles for processing data from RF in one RF pass must be greater than zero.");
	assert(total_compute_cycles_RF_pass > 0 && total_compute_cycles_RF_pass <= total_cycles_RF_pass &&
		"Total cycles for computation on PEs should be greater than zero and not more than cycles for an RF pass.");
}


void baseAccelSim::define_vars_SPM_management()
{
	operand_start_addr_SPM_buffer = new uint32_t*[SPM_buffers];
	for(uint8_t buf=0; buf < SPM_buffers; buf++)
		operand_start_addr_SPM_buffer[buf] = new uint32_t[total_operands]();

	read_operand_SPM_pass											= new bool*[SPM_passes];
	read_operand_offset_base_addr_SPM 				= new std::vector<uint32_t>*[SPM_passes];
	read_operand_burst_width_bytes_SPM_pass		= new std::vector<uint32_t>*[SPM_passes];
	read_operand_offset_base_addr_DRAM				= new std::vector<uint32_t>*[SPM_passes];
	write_operand_SPM_pass										= new bool*[SPM_passes + WB_delay_SPM_pass];
	write_operand_offset_base_addr_SPM			 	= new std::vector<uint32_t>*[SPM_passes + WB_delay_SPM_pass];
	write_operand_burst_width_bytes_SPM_pass	= new std::vector<uint32_t>*[SPM_passes + WB_delay_SPM_pass];
	write_operand_offset_base_addr_DRAM				= new std::vector<uint32_t>*[SPM_passes + WB_delay_SPM_pass];
	for(uint8_t it=0; it < SPM_passes; it++)
	{
		read_operand_SPM_pass[it]											= new bool[total_operands]();
		read_operand_offset_base_addr_SPM[it]					= new std::vector<uint32_t>[total_operands];
		read_operand_burst_width_bytes_SPM_pass[it]		= new std::vector<uint32_t>[total_operands];
		read_operand_offset_base_addr_DRAM[it]				= new std::vector<uint32_t>[total_operands];
	}
	for(uint8_t it=0; it < SPM_passes+WB_delay_SPM_pass; it++)
	{
		write_operand_SPM_pass[it]										= new bool[total_operands]();
		write_operand_offset_base_addr_SPM[it]				= new std::vector<uint32_t>[total_operands];
		write_operand_burst_width_bytes_SPM_pass[it]	= new std::vector<uint32_t>[total_operands];
		write_operand_offset_base_addr_DRAM[it]				= new std::vector<uint32_t>[total_operands];
	}

	spm_buffer_number_op_prefetch		= new uint8_t[total_operands]();
	spm_buffer_number_op_writeback	= new uint8_t[total_operands]();
	spm_buffer_number_op_comp_read	= new uint8_t[total_operands]();
	spm_buffer_number_op_comp_write	= new uint8_t[total_operands]();
}


void baseAccelSim::read_file_SPM_config(string spmConfigFileName)
{
	uint8_t operand_number; uint32_t start_addr_SPM_buffer0, start_addr_SPM_buffer1;
	uint16_t it=0;

	ifstream inFile(spmConfigFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << spmConfigFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3;
		tokenStream >> token1 >> token2 >> token3;
		operand_number = stoul(token1);
		start_addr_SPM_buffer0 = stoul(token2);
		start_addr_SPM_buffer1 = stoul(token3);

		assert((operand_number >= 0 && operand_number < total_operands) && "invalid operand number!");
		assert(start_addr_SPM_buffer0 >= 0 && start_addr_SPM_buffer0 < SPM_size && "invalid SPM address specified!");
		assert(start_addr_SPM_buffer1 >= 0 && start_addr_SPM_buffer1 < SPM_size && "invalid SPM address specified!");

		operand_start_addr_SPM_buffer[0][operand_number] = start_addr_SPM_buffer0;
		operand_start_addr_SPM_buffer[1][operand_number] = start_addr_SPM_buffer1;
		it++;
	}

	inFile.close();
}


void baseAccelSim::read_file_DMAC_prefetch_management(string dmacPrefetchManageFileName)
{
	uint8_t operand_number;	uint16_t SPM_pass_number;
	uint32_t offset_base_addr_SPM,	burst_width_bytes, offset_base_addr_DRAM;
	uint16_t it=0;
	ifstream inFile(dmacPrefetchManageFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << dmacPrefetchManageFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3, token4, token5;
		tokenStream >> token1 >> token2 >> token3 >> token4 >> token5;
		SPM_pass_number = stoul(token1);
		operand_number = stoul(token2);
		offset_base_addr_SPM = stoul(token3);
		burst_width_bytes = stoul(token4);
		offset_base_addr_DRAM = stoul(token5);

		assert((operand_number >= 0 && operand_number < total_operands) && "invalid operand number!");
		assert((SPM_pass_number >= 0 && SPM_pass_number < SPM_passes) && "invalid number for an SPM pass!");
		assert(offset_base_addr_SPM >= 0 && offset_base_addr_SPM < SPM_size &&
			"invalid SPM address specified for setting offset for an operand for an SPM pass!");
		assert(burst_width_bytes > 0 && burst_width_bytes < SPM_size &&
			"invalid burst width specified for managing data in SPM!");
		assert(offset_base_addr_DRAM >= 0 && offset_base_addr_DRAM < SPM_size &&
			"invalid offset specified for accessing operand from DRAM in SPM pass!");

		read_operand_SPM_pass[SPM_pass_number][operand_number] = true;
		read_operand_offset_base_addr_SPM[SPM_pass_number][operand_number].push_back(offset_base_addr_SPM);
		read_operand_burst_width_bytes_SPM_pass[SPM_pass_number][operand_number].push_back(burst_width_bytes);
		read_operand_offset_base_addr_DRAM[SPM_pass_number][operand_number].push_back(offset_base_addr_DRAM);
		it++;
	}

	inFile.close();
}


void baseAccelSim::read_file_DMAC_WB_management(string dmacWBManageFileName)
{
	uint8_t operand_number;	uint16_t SPM_pass_number;
	uint32_t offset_base_addr_SPM,	burst_width_bytes, offset_base_addr_DRAM;
	uint16_t it=0;

	ifstream inFile(dmacWBManageFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << dmacWBManageFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3, token4, token5;
		tokenStream >> token1 >> token2 >> token3 >> token4 >> token5;
		SPM_pass_number = stoul(token1);
		operand_number = stoul(token2);
		offset_base_addr_SPM = stoul(token3);
		burst_width_bytes = stoul(token4);
		offset_base_addr_DRAM = stoul(token5);

		assert((operand_number >= 0 && operand_number < total_operands) && "invalid operand number!");
		assert((SPM_pass_number >= WB_delay_SPM_pass && SPM_pass_number < (SPM_passes+WB_delay_SPM_pass)) && "invalid number for an SPM pass!");
		assert(offset_base_addr_SPM >= 0 && offset_base_addr_SPM < SPM_size &&
			"invalid SPM address specified for setting offset for an operand for an SPM pass!");
		assert(burst_width_bytes > 0 && burst_width_bytes < SPM_size &&
			"invalid burst width specified for managing data in SPM!");
		assert(offset_base_addr_DRAM >= 0 && offset_base_addr_DRAM < SPM_size &&
			"invalid offset specified for accessing operand from DRAM in SPM pass!");

		write_operand_SPM_pass[SPM_pass_number][operand_number] = true;
		write_operand_offset_base_addr_SPM[SPM_pass_number][operand_number].push_back(offset_base_addr_SPM);
		write_operand_burst_width_bytes_SPM_pass[SPM_pass_number][operand_number].push_back(burst_width_bytes);
		write_operand_offset_base_addr_DRAM[SPM_pass_number][operand_number].push_back(offset_base_addr_DRAM);
		it++;
	}

	inFile.close();
}


void baseAccelSim::set_kernel_parameters()
{
	uint16_t idx = 0;
	assert(idx >= 0 && "Not A Valid Processing of the Kernel.");

	std::vector<uint16_t> tokens; std::string token;
	std::istringstream tokenStream(kernels[idx]);
	while (std::getline(tokenStream, token, ','))
	 	tokens.push_back(stoi(token));

	assert(tokens.size() >= 4 && "Incorrect number of bytes for a layer. Total bytes must be 4 or more.");

	total_operands	= tokens[0];
	output_operands = tokens[1];

	assert(total_operands >= 3 && "Total tensor operands must be three or more for a kernel execution.");
	assert(output_operands < total_operands && "Total tensor operands for outputs must be less than total input/output tensor operands for a kernel execution.");

	input_operands = total_operands - output_operands;
	tensor_operands	= new data_type*[total_operands];
	golden_outputs	= new data_type*[output_operands];
	size_tensors		= new uint64_t[total_operands]();

	for(uint8_t it=0; it < total_operands; it++)
	{
		size_tensors[it]	= tokens[2 + it];
		assert(size_tensors[it] >= 1 && "Tensor operand must contain one or more elements.");
		tensor_operands[it]	= new data_type[size_tensors[it]]();
		if(it >= input_operands)
			golden_outputs[it - input_operands] = new data_type[size_tensors[it]]();
	}
}


// Read input operands (tensor data) from file(s)
void baseAccelSim::read_data()
{
	for(uint8_t op=0; op < (total_operands - output_operands); op++)
	{
		string fileName = dir_data + "op" + std::to_string(int(op)+1) + ".txt";
		read_data_operand(op, fileName);
	}
}


void baseAccelSim::read_data_operand(uint8_t op, string fileName) {
	ifstream inFile(fileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << fileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	for(int i=0; i < size_tensors[op]; i++) {
		inFile >> tensor_operands[op][i];
	}
	inFile.close();
}


// Write output operands (tensor data) into file(s)
void baseAccelSim::write_data()
{
	for(uint8_t op=(total_operands-output_operands); op < total_operands; op++)
	{
		string fileName = dir_data + "op" + std::to_string(int(op)+1) + ".txt";
		write_data_operand(op, fileName);
	}
}


void baseAccelSim::write_data_operand(uint8_t op, string fileName)
{
	std::ofstream outFile;
  outFile.open (fileName.c_str(), std::ofstream::out | std::ofstream::trunc);

	for(uint64_t idx = 0; idx < size_tensors[op]; idx++)
    outFile << tensor_operands[op][idx] << "\n";

	outFile.close();
}


// Read data of output operands (tensor data) from file(s)
// This data represents expected values of the outputs
void baseAccelSim::read_golden_outputs()
{
	for(uint8_t op=input_operands; op < total_operands; op++)
	{
		string fileName = dir_data + "golden_op" + std::to_string(int(op)+1) + ".txt";
		ifstream inFile(fileName.c_str());

		if(!inFile)
		{
			cout << "Cannot open and read file: " << fileName.c_str() << endl;
			exit (EXIT_FAILURE);
		}

		for(int i=0; i < size_tensors[op]; i++) {
			inFile >> golden_outputs[op-input_operands][i];
		}
		inFile.close();
	}
}

// compare generated output tensors with expected data provided as reference.
// Raise error if there is a mismatch.
void baseAccelSim::compare_golden_outputs()
{
	if(compare_golden)
	{
		read_golden_outputs();

		for(uint8_t op=(total_operands-output_operands); op < total_operands; op++)
		{
			for(int i=0; i < size_tensors[op]; i++)
			{
				if(golden_outputs[op-input_operands][i] - tensor_operands[op][i] > 0.001)
				{
					printf("Operand %d: Golden output does not match with generated output.\n", op);
					exit (EXIT_FAILURE);
				}
			}
		}
		cout << "Generated output operands consist of expected values.\n";
	}
}

void baseAccelSim::read_file_network_info(string networkInfoFileName)
{
	total_MC = nRows_PE_Array*nCols_PE_Array + nRows_PE_Array;

	size_packetArr = new uint16_t[total_operands]();
	size_config_data_collect_write_back = new uint16_t[outNetworks]();

	uint16_t it=0;
	ifstream inFile(networkInfoFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << networkInfoFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		istringstream tokenStream(line);
		string token1;
		tokenStream >> token1;
		int idx_output_network = it - (total_operands+1);

		if(it == 0)
			PE_trigger_latency = stoul(token1);
		else if(it <= total_operands)
			size_packetArr[it-1] = stoul(token1);
		else if(idx_output_network <= outNetworks)
			size_config_data_collect_write_back[idx_output_network] = stoul(token1);
		it++;
	}
	inFile.close();
}


void baseAccelSim::read_file_config_interconnect(string interconnectConfigFileName)
{
	configArr = new Packet**[totalNetworks];
	for(uint8_t i=0; i < totalNetworks; i++)
  {
    configArr[i] = new Packet*[total_MC];
    // each Multicast Controller requires 4 parameters for configured
    for(uint16_t j=0; j < total_MC; j++)
      configArr[i][j] = new Packet[network_controller_config_params];
  }

	uint16_t it=0;
	ifstream inFile(interconnectConfigFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << interconnectConfigFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		istringstream tokenStream(line);
		string token1, token2, token3, token4, token5, token6;
		tokenStream >> token1 >> token2 >> token3 >> token4 >> token5 >> token6;
		uint8_t 	network 	= stoul(token1);
		uint16_t	idx				= stoul(token2);
		uint8_t		val				= stoul(token3);
		uint8_t 	rowTag 		= stoul(token4);
		uint8_t 	colTag 		= stoul(token5);
		data_type configVal	= stoul(token6);

		configArr[network][idx][val].setData(configVal);
		configArr[network][idx][val].setRowTag(rowTag);
		configArr[network][idx][val].setColTag(colTag);
	}
	inFile.close();
}


void baseAccelSim::read_file_input_network_packets(string inputNetworkPacketsFileName)
{
	packetArr = new Packet*[total_operands];
	for(uint8_t i=0; i < total_operands; i++)
    packetArr[i] = new Packet[size_packetArr[i]];

	uint16_t it=0;
	ifstream inFile(inputNetworkPacketsFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << inputNetworkPacketsFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		istringstream tokenStream(line);
		string token1, token2, token3, token4;
		tokenStream >> token1 >> token2 >> token3 >> token4;
		uint8_t		operand 	= stoul(token1);
		uint16_t 	idx				= stoul(token2);
		uint8_t 	rowTag 		= stoul(token3);
		uint8_t		colTag 		= stoul(token4);

		packetArr[operand][idx].setData(0);
		packetArr[operand][idx].setRowTag(rowTag);
		packetArr[operand][idx].setColTag(colTag);
	}
	inFile.close();
}


void baseAccelSim::read_file_output_network_packets(string outputNetworkPacketsFileName)
{
	config_data_collect_write_back = new uint16_t*[outNetworks];
	for(uint8_t i=0; i < outNetworks; i++)
    config_data_collect_write_back[i] = new uint16_t[size_config_data_collect_write_back[i]];

	uint16_t it=0;
	ifstream inFile(outputNetworkPacketsFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << outputNetworkPacketsFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3;
		tokenStream >> token1 >> token2 >> token3;

		uint8_t		network 	= stoul(token1);
		uint16_t 	idx				= stoul(token2);
		uint16_t 	offset 		= stoul(token3);

		assert(network < outNetworks && "network number should be less than total networks for outputting data.");
		assert(idx < size_config_data_collect_write_back[network] &&
			"packet number should be less than total data collected via network in an execution pass.");
		assert(offset < size_config_data_collect_write_back[network] &&
			"packet's offset number should be less than total data collected via network in an execution pass.");

		config_data_collect_write_back[network][idx] = offset;
	}
	inFile.close();
}


void baseAccelSim::define_vars_network_comm_management()
{
	read_data_RF_pass																		= new bool*[RF_passes];
	RF_pass_network_read_operand												= new std::vector<uint8_t>*[RF_passes];
	RF_pass_network_read_operand_offset_network_packet	= new std::vector<uint16_t>*[RF_passes];
	RF_pass_network_read_operand_burst_width_elements		= new std::vector<uint32_t>*[RF_passes];
	RF_pass_network_read_operand_offset_base_addr_SPM		= new std::vector<uint32_t>*[RF_passes];
	write_data_RF_pass																	= new bool*[RF_passes + WB_delay_RF_pass];
	RF_pass_network_write_operand												= new std::vector<uint8_t>*[RF_passes + WB_delay_RF_pass];
	RF_pass_network_write_operand_offset_network_packet	= new std::vector<uint16_t>*[RF_passes + WB_delay_RF_pass];
	RF_pass_network_write_operand_burst_width_elements	= new std::vector<uint32_t>*[RF_passes + WB_delay_RF_pass];
	RF_pass_network_write_operand_offset_base_addr_SPM	= new std::vector<uint32_t>*[RF_passes + WB_delay_RF_pass];
	for(uint8_t it=0; it < RF_passes; it++)
	{
		read_data_RF_pass[it]																		= new bool[inNetworks]();
		RF_pass_network_read_operand[it]												= new std::vector<uint8_t>[inNetworks];
		RF_pass_network_read_operand_offset_network_packet[it]	= new std::vector<uint16_t>[inNetworks];
		RF_pass_network_read_operand_burst_width_elements[it]		= new std::vector<uint32_t>[inNetworks];
		RF_pass_network_read_operand_offset_base_addr_SPM[it]		= new std::vector<uint32_t>[inNetworks];
	}
	for(uint8_t it=0; it < RF_passes + WB_delay_RF_pass; it++)
	{
		write_data_RF_pass[it]																	= new bool[outNetworks]();
		RF_pass_network_write_operand[it]												= new std::vector<uint8_t>[outNetworks];
		RF_pass_network_write_operand_offset_network_packet[it]	= new std::vector<uint16_t>[outNetworks];
		RF_pass_network_write_operand_burst_width_elements[it]	= new std::vector<uint32_t>[outNetworks];
		RF_pass_network_write_operand_offset_base_addr_SPM[it]	= new std::vector<uint32_t>[outNetworks];
	}
}


void baseAccelSim::read_file_SPM_read_management(string spmReadManageFileName)
{
	uint8_t operand_number, network_number;	uint16_t RF_pass_number, offset_network_packets;
	uint32_t offset_base_addr_SPM,	burst_width_elements;
	uint16_t it=0;
	ifstream inFile(spmReadManageFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << spmReadManageFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3, token4, token5, token6;
		tokenStream >> token1 >> token2 >> token3 >> token4 >> token5 >> token6;
		RF_pass_number 					= stoul(token1);
		network_number					= stoul(token2);
		operand_number 					= stoul(token3);
		offset_network_packets	= stoul(token4);
		burst_width_elements		= stoul(token5);
		offset_base_addr_SPM 		= stoul(token6);

		assert((RF_pass_number >= 0 && RF_pass_number < RF_passes) && "invalid number for an RF pass!");
		assert((network_number >= 0 && network_number < inNetworks) && "invalid number of a network for read-operands!");
		assert((operand_number >= 0 && operand_number < total_operands) && "invalid operand number!");
		assert(offset_network_packets >= 0 && offset_network_packets < size_packetArr[operand_number] &&
			"invalid start address specified for communicating data packets for a read-operand in an RF pass!");
		assert(burst_width_elements > 0 && burst_width_elements <= size_packetArr[operand_number] &&
			"invalid burst width specified for communicating data packets for read-operand via network!");
			assert(offset_base_addr_SPM >= 0 && offset_base_addr_SPM < SPM_size &&
				"invalid SPM address specified for reading an operand in an RF pass!");

		read_data_RF_pass[RF_pass_number][network_number] = true;
		RF_pass_network_read_operand[RF_pass_number][network_number].push_back(operand_number);
		RF_pass_network_read_operand_offset_network_packet[RF_pass_number][network_number].push_back(offset_network_packets);
		RF_pass_network_read_operand_burst_width_elements[RF_pass_number][network_number].push_back(burst_width_elements);
		RF_pass_network_read_operand_offset_base_addr_SPM[RF_pass_number][network_number].push_back(offset_base_addr_SPM);
		it++;
	}
	inFile.close();
}


void baseAccelSim::read_file_SPM_write_management(string spmWriteManageFileName)
{
	uint8_t operand_number, network_number;	uint16_t RF_pass_number, offset_network_packets;
	uint32_t offset_base_addr_SPM,	burst_width_elements;
	uint16_t it=0;

	ifstream inFile(spmWriteManageFileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << spmWriteManageFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3, token4, token5, token6;
		tokenStream >> token1 >> token2 >> token3 >> token4 >> token5 >> token6;
		RF_pass_number 					= stoul(token1);
		network_number					= stoul(token2);
		uint8_t	write_network		= inNetworks + network_number;
		operand_number 					= stoul(token3);
		offset_network_packets	= stoul(token4);
		burst_width_elements		= stoul(token5);
		offset_base_addr_SPM 		= stoul(token6);

		assert((RF_pass_number >= WB_delay_RF_pass && RF_pass_number < RF_passes + WB_delay_RF_pass) && "invalid number for an RF pass!");
		assert((network_number >= 0 && network_number < outNetworks) && "invalid number of a network for write-operands!");
		assert((operand_number >= 0 && operand_number < total_operands) && "invalid operand number!");
		assert(offset_network_packets >= 0 && offset_network_packets < size_packetArr[operand_number] &&
			"invalid start address specified for communicating data packets for a write-operand in an RF pass!");
		assert(burst_width_elements > 0 && burst_width_elements <= size_packetArr[operand_number] &&
			"invalid burst width specified for communicating data packets for write-operand via network!");
			assert(offset_base_addr_SPM >= 0 && offset_base_addr_SPM < SPM_size &&
				"invalid SPM address specified for writing an operand in an RF pass!");

		write_data_RF_pass[RF_pass_number][network_number] = true;
		RF_pass_network_write_operand[RF_pass_number][network_number].push_back(operand_number);
		RF_pass_network_write_operand_offset_network_packet[RF_pass_number][network_number].push_back(offset_network_packets);
		RF_pass_network_write_operand_burst_width_elements[RF_pass_number][network_number].push_back(burst_width_elements);
		RF_pass_network_write_operand_offset_base_addr_SPM[RF_pass_number][network_number].push_back(offset_base_addr_SPM);
		it++;
	}
	inFile.close();
}


void baseAccelSim::read_file_instns(string fileName)
{
	ifstream inFile(fileName.c_str());
	if(!inFile)
	{
		cout << "Cannot open and read file: " << fileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
	while (getline(inFile,line))
  {
		instArr.push_back(stoull(line));
  }
  inFile.close();
}


void baseAccelSim::read_file_config_trigger_PEs(string configTriggerPEsFileName)
{
	trigger_exec_pass_PEs_exec				= new bool*[SPM_passes + WB_delay_SPM_pass];
	trigger_exec_pass_zero_init				= new bool*[SPM_passes + WB_delay_SPM_pass];
	trigger_exec_pass_writeback				= new bool*[SPM_passes + WB_delay_SPM_pass];
	trigger_exec_pass_spatial_reduce	= new bool*[SPM_passes + WB_delay_SPM_pass];

	trigger_exec_pass_update_buf_num_Op1	= new bool*[SPM_passes + WB_delay_SPM_pass];
	trigger_exec_pass_update_buf_num_Op2	= new bool*[SPM_passes + WB_delay_SPM_pass];
	trigger_exec_pass_update_buf_num_Op3	= new bool*[SPM_passes + WB_delay_SPM_pass];

	for(uint8_t it=0; it < RF_passes + WB_delay_RF_pass; it++)
	{
		trigger_exec_pass_PEs_exec[it]				= new bool[RF_passes + WB_delay_RF_pass]();
		trigger_exec_pass_zero_init[it]				= new bool[RF_passes + WB_delay_RF_pass]();
		trigger_exec_pass_writeback[it]				= new bool[RF_passes + WB_delay_RF_pass]();
		trigger_exec_pass_spatial_reduce[it]	= new bool[RF_passes + WB_delay_RF_pass]();
		trigger_exec_pass_update_buf_num_Op1[it]	= new bool[RF_passes + WB_delay_RF_pass]();
		trigger_exec_pass_update_buf_num_Op2[it]	= new bool[RF_passes + WB_delay_RF_pass]();
		trigger_exec_pass_update_buf_num_Op3[it]	= new bool[RF_passes + WB_delay_RF_pass]();
	}

	uint16_t RF_pass_number, SPM_pass_number;
	bool PEs_exec, zero_init, write_back, spatial_reduce;
	bool update_buf_num_Op1, update_buf_num_Op2, update_buf_num_Op3;
	uint16_t it=0;
	ifstream inFile(configTriggerPEsFileName.c_str());

	if(!inFile)
	{
		cout << "Cannot open and read file: " << configTriggerPEsFileName.c_str() << endl;
		exit (EXIT_FAILURE);
	}

	string line;
 	while (getline(inFile, line)) {
		// skip reading first two lines of the file
		if(it < 2) {
			it++; continue;
		}
		istringstream tokenStream(line);
		string token1, token2, token3, token4, token5, token6, token7, token8, token9;
		tokenStream >> token1 >> token2 >> token3 >> token4 >> token5 >> token6
			>> token7 >> token8 >> token9;

		SPM_pass_number = stoul(token1);
		RF_pass_number	= stoul(token2);
		PEs_exec				= stoul(token3);
		zero_init 			= stoul(token4);
		write_back 			= stoul(token5);
		spatial_reduce	= stoul(token6);
		update_buf_num_Op1 = stoul(token7);
		update_buf_num_Op2 = stoul(token8);
		update_buf_num_Op3 = stoul(token9);

		assert((SPM_pass_number >= 1 && SPM_pass_number < (SPM_passes+1)) && "invalid number for an SPM pass!");
		assert((RF_pass_number >= 1 && RF_pass_number <= (RF_passes+1)) && "invalid number for an RF pass!");

		trigger_exec_pass_PEs_exec[SPM_pass_number][RF_pass_number] = PEs_exec;
		trigger_exec_pass_zero_init[SPM_pass_number][RF_pass_number] = zero_init;
		trigger_exec_pass_writeback[SPM_pass_number][RF_pass_number] 	= write_back;
		trigger_exec_pass_spatial_reduce[SPM_pass_number][RF_pass_number] = spatial_reduce;
		trigger_exec_pass_update_buf_num_Op1[SPM_pass_number][RF_pass_number] = update_buf_num_Op1;
		trigger_exec_pass_update_buf_num_Op2[SPM_pass_number][RF_pass_number] = update_buf_num_Op2;
		trigger_exec_pass_update_buf_num_Op3[SPM_pass_number][RF_pass_number] = update_buf_num_Op3;
		it++;
	}
	inFile.close();
}


void baseAccelSim::read_accel_configurations()
{
	read_file_instns(instnsFileName);
	define_vars_SPM_management();
	read_file_SPM_config(spmConfigFileName);
	read_file_DMAC_prefetch_management(dmacPrefetchManageFileName);
	read_file_DMAC_WB_management(dmacWBManageFileName);
	read_file_network_info(networkInfoFileName);
	read_file_config_interconnect(interconnectConfigFileName);
	read_file_input_network_packets(inputNetworkPacketsFileName);
	read_file_output_network_packets(outputNetworkPacketsFileName);
	define_vars_network_comm_management();
	read_file_SPM_read_management(spmReadManageFileName);
	read_file_SPM_write_management(spmWriteManageFileName);
	read_file_config_trigger_PEs(configTriggerPEsFileName);
}


void baseAccelSim::free_memory()
{
	delete[] regs_buf1_startIdx_Op;
	delete[] regs_buf2_startIdx_Op;

	for(uint8_t it=0; it < SPM_passes; it++)
	{
		delete[] read_operand_SPM_pass[it];
		delete[] read_operand_offset_base_addr_SPM[it];
		delete[] read_operand_burst_width_bytes_SPM_pass[it];
		delete[] read_operand_offset_base_addr_DRAM[it];
	}
	for(uint8_t it=SPM_passes; it < SPM_passes + WB_delay_SPM_pass; it++)
	{
		delete[] write_operand_SPM_pass[it];
		delete[] write_operand_offset_base_addr_SPM[it];
		delete[] write_operand_burst_width_bytes_SPM_pass[it];
		delete[] write_operand_offset_base_addr_DRAM[it];
	}
	delete[] read_operand_SPM_pass;
	delete[] read_operand_offset_base_addr_SPM;
	delete[] read_operand_burst_width_bytes_SPM_pass;
	delete[] read_operand_offset_base_addr_DRAM;
	delete[] write_operand_SPM_pass;
	delete[] write_operand_offset_base_addr_SPM;
	delete[] write_operand_burst_width_bytes_SPM_pass;
	delete[] write_operand_offset_base_addr_DRAM;
	delete[] spm_buffer_number_op_prefetch;
	delete[] spm_buffer_number_op_writeback;
	delete[] spm_buffer_number_op_comp_read;
	delete[] spm_buffer_number_op_comp_write;


	for(uint8_t buf=0; buf < SPM_buffers; buf++)
		delete[] operand_start_addr_SPM_buffer[buf];
	delete[] operand_start_addr_SPM_buffer;

	for(uint8_t i=0; i<total_operands; i++)
		delete[] tensor_operands[i];

	for(uint8_t i=0; i<output_operands; i++)
		delete[] golden_outputs[i];

	delete[] tensor_operands;
	delete[] golden_outputs;
	delete[] size_tensors;

	for(uint8_t i=0; i < total_operands; i++)
    delete[] packetArr[i];
  delete[] packetArr;

  for(uint8_t i=0; i < totalNetworks; i++) {
    for(uint16_t j=0; j < total_MC; j++) {
      delete[] configArr[i][j];
    }
    delete[] configArr[i];
  }
	delete [] configArr;

  for(uint8_t i=0; i < outNetworks; i++)
		delete[] config_data_collect_write_back[i];
	delete[] config_data_collect_write_back;

	delete[] size_packetArr;
	delete[] size_config_data_collect_write_back;

	for(uint8_t it=0; it < RF_passes; it++)
	{
		delete[] read_data_RF_pass[it];
		delete[] RF_pass_network_read_operand[it];
		delete[] RF_pass_network_read_operand_offset_network_packet[it];
		delete[] RF_pass_network_read_operand_burst_width_elements[it];
		delete[] RF_pass_network_read_operand_offset_base_addr_SPM[it];
	}
	for(uint8_t it=RF_passes; it < RF_passes + WB_delay_RF_pass; it++)
	{
		delete[] write_data_RF_pass[it];
		delete[] RF_pass_network_write_operand[it];
		delete[] RF_pass_network_write_operand_offset_network_packet[it];
		delete[] RF_pass_network_write_operand_burst_width_elements[it];
		delete[] RF_pass_network_write_operand_offset_base_addr_SPM[it];
	}
	delete[] read_data_RF_pass;
	delete[] RF_pass_network_read_operand;
	delete[] RF_pass_network_read_operand_offset_network_packet;
	delete[] RF_pass_network_read_operand_burst_width_elements;
	delete[] RF_pass_network_read_operand_offset_base_addr_SPM;
	delete[] write_data_RF_pass;
	delete[] RF_pass_network_write_operand;
	delete[] RF_pass_network_write_operand_offset_network_packet;
	delete[] RF_pass_network_write_operand_burst_width_elements;
	delete[] RF_pass_network_write_operand_offset_base_addr_SPM;

	for(uint8_t it=0; it < RF_passes + WB_delay_RF_pass; it++)
	{
		delete[] trigger_exec_pass_PEs_exec[it];
		delete[] trigger_exec_pass_zero_init[it];
		delete[] trigger_exec_pass_writeback[it];
		delete[] trigger_exec_pass_spatial_reduce[it];
		delete[] trigger_exec_pass_update_buf_num_Op1[it];
		delete[] trigger_exec_pass_update_buf_num_Op2[it];
		delete[] trigger_exec_pass_update_buf_num_Op3[it];
	}
	delete[] trigger_exec_pass_PEs_exec;
	delete[] trigger_exec_pass_zero_init;
	delete[] trigger_exec_pass_writeback;
	delete[] trigger_exec_pass_spatial_reduce;
	delete[] trigger_exec_pass_update_buf_num_Op1;
	delete[] trigger_exec_pass_update_buf_num_Op2;
	delete[] trigger_exec_pass_update_buf_num_Op3;
}


int main(int argc, const char** argv)
{
	baseAccelSim accel;

	// parse arguments from command line
	parse_arguments(argc, argv);

	accel.read_inputs();
	accel.read_data();
	accel.read_accel_configurations();

	controller accelController;
	accelController.run(tensor_operands);

	printf("Done!! ticks are: %ld \n",tick);
	accel.write_data();
	accel.compare_golden_outputs();
	accel.free_memory();

	return 0;
}
