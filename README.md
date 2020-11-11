# DiRAC

DiRAC is a cycle-level microarchitectural simulator for dataflow accelerators. Programmers can specify different kernel functionalies, schedules for architectural executions, and architecture variants, and can observe performance implications. It allows us to learn about how dataflow accelerators work, how we can validate our execution method (mappings) onto the accelerator with cycle-level details, a reconfigurable accelerator template (instead of building such a design from scratch), and easy prototyping and sensitivity analysis of the accelerator design features. 


## Installation
1. clone this repository with `--recursive` option. 

    ```bash
    git clone --recursive <repository-url>
    ```

2. Navigate to the top of the directory and build it. 

  ```bash
  cd /path/to/DiRAC/
  make
  ```


## High-level Overview

<p align="center">
  <img src="https://labs.engineering.asu.edu/mps-lab/wp-content/uploads/sites/8/2019/10/DiRAC_Overview.png"/ height="250">
</p>

Above figure provides a high-level overview of the simulator. Programmer or the compiler can specify the input kernel and accelerator architecture description, i.e., pipelining of the PEs, configuration of the memories, and interconnect topology. For the target kernel (nested loops), programmer also specifies configurations that corresponds to details (machine instructions) like how to manage the PEs, and schedule data transfers for memories and interconnect, and so forth. These are the details for an execution method or the mapping of the loops onto target architecture. Then, the simulator can execute cycle-level functionalities of the microarchitectural blocks and then it outputs execution cycles, and the resultant data. If the expected output data is provided, then the simulator can compare it with the generated output and inform us whether the simulation has terminated with a successful functionality.


## Exploring Dataflow Accelerations of Loops through DiRAC

The steps for executing the loops of the applications with DiRAC are as follows:

1.	**Setup the Test**

For simulating the execution (of target kernel on accelerator architecture), the programmer should specify the details in a required format. For example, for the test `tests/test1/`, follwing is the structure of a test directory.  

```
test1/kernel - contains code (configurations or machine instructions) of the kernel for programming the hardware
test1/arch   - specify accelerator architecture
test1/data   - input data for input operands (optionally, expected output data)
```

For more details, please refer to `docs`. There are a few example tests provided in the `tests` directory, which can help understanding how to setup the test, in particular the code for the kernel.  


2. **Simulate Accelerator Execution**: 

Please run

```
./out/DiRAC --path /path/to/test/directory/ [-cmp-golden 1]
```

For example, test of executing convolution with output stationary dataflow on a small data can be executed with the command:

```./out/DiRAC --path ./tests/convolution_output_stationary -cmp-golden 1```

Test `convolution_output_stationary` provides an example on simulating convolution of 4 input images of 3 channels with weights of 8 filters on an architecture with 9 PEs. The simulation results into the correct output for 4 output images with 8 output channels. This test configures the scheduling of data transfers such that each PE receives some 3 cross 3 values from each channel of an image and along with the filter data; each PE computes one output value among 3x3 2D output. Since input images are of larger sizes, scratchpad and DRAM access schedules ensure that image data is reused. 

**Output:**
```
Done! ticks are: 1788
Generated output operands consist of expected values.
```

This implies that the execution of a total of 7776 multiply-and-accumulate operations onto 9-PE design terminate in 1786 cycles.
Please note that the final execution cycles accounts for loading data from DRAM to the scratchpad, from scratchpad to PE via interconnect, PE computations, and writeback from PE to scratchpad via interconnect, and finally a write back to DRAM. In fact, all of these events may occur many times (reffered to as multiple RF and SPM passes). We can see that our simulation resulted into correct output, which can be found in `./tests/convolution_output_stationary/data/`.

Thus, with DiRAC, we can simulate execution of any neted loop with arbitrary number of data operands. Current limitations are that all loop operations should be expressed as a multiply and accumulate (since PE functionality is currently limited to a MAC unit), and that loops should not feature any conditional statements i.e., no if then else statements or swtich case. Our future enhancements are planned to integrate additional architectural features, a compiler support for programming the architecture, and on-device architecture emulation for cycle-accurate validation. 


3. **Try Out a Different Mapping and Observe Performance Implications**

Execution methods can significantly impact the efficiency of the acceleration. Execution methods can vary in terms of scheduling data transfers differently for accessing off-chip memory, or via interconnect to PEs, or by communicating the data to PEs in a different manner and making PEs process different dataflow, and so forth. So, try setting up a valid kernel code (configuration) by modifying one or more files in `test1/kernel/`, and then simulate again. The simulation errors may assist you to determine whether your modification of kernel code is illegal or somehow results in an incorrect functionality. 

To learn about how to efficiently map loops onto programmable dataflow accelerators including optimizing different Deep Neural Network models, please refer to the following publication.
 
Shail Dave, Youngbin Kim, Sasikanth Avancha, Kyoungwoo Lee, Aviral Shrivastava, [dMazeRunner: Executing Perfectly Nested Loops on Dataflow Accelerators](https://dl.acm.org/doi/pdf/10.1145/3358198), in ACM Transactions on Embedded Computing Systems (TECS) \[Special Issue on ESWEEK 2019 - ACM/IEEE International Conference on Hardware/Software Codesign and System Synthesis (CODES+ISSS)\]. \[[Repo](https://github.com/MPSLab-ASU/dMazeRunner)\]


### Contact Information

For any questions or comments on DiRAC, please email us at cmlasu@gmail.com
