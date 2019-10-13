#include "parse.hh"
#include "../argparse/argparse.hpp"

void parse_arguments(int argc, const char** argv)
{
  // make a new ArgumentParser
  // Argument format: short_name, long_name, total_arg_vals, is_optional
  ArgumentParser parser;

  // Argument that sets the path of the test directory
  parser.addArgument("-p", "--path", 1, false);
  parser.addArgument("-c", "--cmp-golden", 1, true);

  // parse the command-line arguments - throws error if invalid format
  if(argc < 3)
  {
    std::cout << parser.usage() << std::endl;
    exit (EXIT_FAILURE);
  }

  parser.parse(argc, argv);

  // Arguments are successfully parsed through command-line.
  // Use them to set necessary variables.
  dir_test = parser.retrieve<std::string>("path");

  if(parser.count("cmp-golden") == 1)
    compare_golden = true;
  else
    compare_golden = false;

  setup_file_paths();
}

void setup_file_paths()
{
  dir_kernel = dir_test + "kernel/";
	dir_arch = dir_test + "arch/";
  dir_data = dir_test + "data/";

  // Programmer specifies architecture specification
	archSpecFileName = dir_arch + "accelerator.txt";

  // Kernel information, including machine instructions to program
  // PEs, memories, interconnect, DMA controller. 
  // To be provided by programmer or compiler
	kernelFileName = dir_kernel + "kernels.txt";
  configFileName = dir_kernel + "config_PEs.txt";
	instnsFileName = dir_kernel + "PEInstns.txt";
	spmConfigFileName = dir_kernel + "data_manage_SPM.txt";
	dmacPrefetchManageFileName = dir_kernel + "DMAC_prefetch_SPM_passes.txt";
	dmacWBManageFileName = dir_kernel + "DMAC_WB_SPM_passes.txt";
	networkInfoFileName = dir_kernel + "interconnect_info.txt";
	interconnectConfigFileName = dir_kernel + "config_network_controllers.txt";
	inputNetworkPacketsFileName = dir_kernel + "data_packets_interconnect.txt";
	outputNetworkPacketsFileName = dir_kernel + "data_packets_interconnect_write_back.txt";
	spmReadManageFileName = dir_kernel + "SPM_read_RF_passes.txt";
	spmWriteManageFileName = dir_kernel + "SPM_write_RF_passes.txt";
	configTriggerPEsFileName = dir_kernel + "config_trigger_PEs.txt";
}
