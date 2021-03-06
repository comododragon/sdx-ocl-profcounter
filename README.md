# SDx OpenCL ProfCounter

OpenCL Fine-grain Profiling Kernel for Xilinx SDx Development Environment

## Authors

* André Bannwart Perina

## Introduction

The ProfCounter is a framework developed for fine-grain profiling of OpenCL kernels developed using the Xilinx SDx environment. It consists of:

* A header that may be included to your design under test (DUT, namely an OpenCL kernel), including functions for timestamping;
* An RTL kernel that performs cycle count.

Using the timestamping function, one can precisely count the cycles of a specific region of an OpenCL kernel, as the RTL kernel is driven by the same clock as the DUT kernel.

## Licence

Most of this project is distributed under the BSD-3-Clause licence. See LICENSE.TXT for details.

The ```include/common.h``` header is distributed under the GPL-3.0 licence. See the following repo for details: https://github.com/comododragon/commonh

The BFS example kernel is adapted from the Rodinia Benchmark Suite (https://rodinia.cs.virginia.edu/doku.php), therefore it holds the Rodinia licence.

## Usage Introduction

This repository provides a functional example project of ProfCounter at folder ***base***. It consists of two kernels:

* ***src/probe.cl:*** a very simple kernel that perform some logic at certain times:
```
__kernel void probe(__global unsigned * restrict timeline) {
	int i, j;

	for(i = 0, j = 0; i < 10; j++) {
		if(timeline[i] == j) {
			// Do something...
			i++;
		}
	}
}
```

* ***src/profCounter/:*** the RTL kernel that performs all the magic. This is the pseudo-code for the kernel (the hold logic is not shown for simplicity):
```
#define COMM_NOP 0x0
#define COMM_STAMP 0xD
#define COMM_HOLD 0xE
#define COMM_FINISH 0xF

__read_only pipe unsigned p0 __attribute__((xcl_reqd_pipe_depth(16)));

__kernel void profCounter(__global long * restrict log) {
	long *offset = 0x0;
	unsigned command = COMM_NOP;

	for(long cycleCount = 0; command != COMM_FINISH; cycleCount++) {
		read_pipe(p0, &command);

		if(COMM_STAMP == command)
			log[offset++] = cycleCount;
		else if(command > COMM_NOP && command < COMM_STAMP)
			log[offset++] = (((command - 1) & 0xF) << 60) | (cycleCount & 0x0FFFFFFFFFFFFFFF);
	}
}
```

This pseudo-kernel is just for illustration purposes. As of Xilinx SDx 2018.2, non-blocking ```read_pipe``` is not implemented, therefore the cycle count is infeasible to be performed accurately, thus the kernel was implemented in Verilog following the same logic as presented above, though pipe reading and timestamping are performed in parallel and non-blocking. Thus, the behaviour if this kernel is:

* It starts cycle counting as soon as the kernel is launched using ```clEnqueueTask```;
* It saves a timestamp on the global memory space when a ```COMM_STAMP``` command is received through the ```p0``` pipe;
* It saves a timestamp plus a checkpoint ID if a ```COMM_CHECKPOINT``` command is issued (in current implementation, there are 12 different checkpoint IDs, where command ```0x1``` issues a checkpoint with ID 0, ```0x2``` a checkpoint with ID 1 and so on);
* If ```COMM_HOLD``` is issued, all following stamps and checkpoints are only saved to global memory after ```COMM_FINISH``` is called;
* It stops counting when a ```COMM_FINISH``` command is received through the ```p0```pipe.

Therefore, to use the profiler on our ```probe```, some simple modifications are needed:
```
#include "profcounter.h"

__kernel void probe(__global unsigned * restrict timeline) {
	int i, j;

	PROFCOUNTER_INIT();

	for(i = 0, j = 0; i < 10; j++) {
		if(timeline[i] == j) {
			PROFCOUNTER_STAMP();
			// Do something...
			i++;
		}
	}

	PROFCOUNTER_FINISH();
}
```

You must ensure that on the host code, the ```profCounter``` gets started BEFORE ```probe```. The ```include/profcounter.h``` is a very simple header that already provides the pipe declaration for your DUT and macros that writes commands to the pipe. You must also ensure that the ```COMM_FINISH``` command is sent to the profiler (using ```PROFCOUNTER_FINISH()``` for convenience), otherwise the profiling kernel will execute indefinitely and likely to hang your host code execution.

## Usage by Example

The example presented in the previous section is already implemented in the example project provided with this repository. The project was tested on a Xilinx Zynq UltraScale+ zcu102 board. In order to compile this example:

* Certify that you have a valid installation of Xilinx SDx Environment (the version used was 2018.2, other versions are untested). You need valid licences also for OpenCL compilation, either ```sdsoc``` or ```sdaccel``` licence;
* Certify that your Xilinx environments are set up correctly. Usually it is as simple as sourcing the settings script:
```
$ source /path/to/xilinx/SDx/2018.2/settings64.sh
```
* Run make (actual synthesis is performed here so grab a cup of tea, listen to some podcasts, play a guitar, etc.):
```
$ make hw
```
* Copy the contents of ```fpga/hw/<PLATFORM>/sd_card``` to an SD card, insert on your Zynq UltraScale+ board and boot from SD;
	* Make sure that your board is configured to boot from SD card. See the User Guide of your board for further details;
* Boot the board;
* Open a serial connection to a terminal on the board;
* The ```init.sh``` already handles exporting the necessary environment variables and cd'ing to ```/mnt```. Thus all you need is to execute the host code:
```
$ ./execute
```
* With this example code, timestamping will be requested when ```j``` is 0, 15, 30, 40, 50, 70, 100, 120, 199 and 200 on the ```probe``` kernel. Considering that this loop has a latency of 137 cycles, the following output is expected:
```
[ OK ] Getting platforms IDs...
[ OK ] Getting devices IDs for first platform...
[ OK ] Creating context...
[ OK ] Creating command queue for "profCounter"...
[ OK ] Creating command queue for "probe"...
[ OK ] Opening program binary...
[ OK ] Reading program binary...
[ OK ] Creating program from binary...
[ OK ] Building program...
[ OK ] Creating kernel "profCounter" from program...
[ OK ] Creating kernel "probe" from program...
[ OK ] Creating buffers...
[ OK ] Setting kernel arguments for "profCounter"...
[ OK ] Setting kernel arguments for "probe"...
[ OK ] [0] Setting buffers...
[ OK ] [0] Running kernels...
[ OK ] [0] Getting kernels arguments...
Elapsed time spent on kernels: 476 us; Average time per iteration: 476.000000 us.
Received values (assuming latency of 137 cycles):
|    | Absolute values                       || Latency-normalised values             | ID (if      |
|  i |       t(i) |  t(i)-t(0) | t(i)-t(i-1) ||       t(i) |  t(i)-t(0) | t(i)-t(i-1) | applicable) |
|  0 |   75004092 |          0 |           0 ||     547475 |          0 |           0 |           0 |
|  1 |   75004229 |        137 |         137 ||     547476 |          1 |           1 |          10 |
|  2 |   75006284 |       2192 |        2055 ||     547491 |         16 |          15 |          10 |
|  3 |   75008339 |       4247 |        2055 ||     547506 |         31 |          15 |          10 |
|  4 |   75009709 |       5617 |        1370 ||     547516 |         41 |          10 |          10 |
|  5 |   75011079 |       6987 |        1370 ||     547526 |         51 |          10 |          10 |
|  6 |   75013819 |       9727 |        2740 ||     547546 |         71 |          20 |          10 |
|  7 |   75017929 |      13837 |        4110 ||     547576 |        101 |          30 |          10 |
|  8 |   75020669 |      16577 |        2740 ||     547596 |        121 |          20 |          10 |
|  9 |   75031492 |      27400 |       10823 ||     547675 |        200 |          79 |          10 |
| 10 |   75031629 |      27537 |         137 ||     547676 |        201 |           1 |          10 |
| 11 |   75031630 |      27538 |           1 ||     547676 |        201 |           0 |          11 |
```
* ***Note:*** you can execute the host code with ```hold``` argument to activate the hold behaviour of ProfCounter (see Section ***ProfCounter Commands***):
```
$ ./execute hold
```

## Scheduling Issues

ProfCounter is a measuring tool, thus its use must not change the latency of the final hardware. We noticed some problems under certain conditions that the CFG of the high-level code was affected just by the presence of ```write_pipe()``` builtin functions in the DUT, as the ```PROFCOUNTER_*()``` macros are in fact substituted by ```write_pipe()``` calls.

To overcome this issue, the ```PROFCOUNTER_*()``` calls (except ```PROFCOUNTER_FINISH()```) instead insert a placeholder command, which is substituted by the actual ```write_pipe()``` calls after the code is optimised and right before the HLS scheduling/binding. This is performed by the ```directives.tcl``` and ```transform.sh``` scripts.

However, if no ```write_pipe()``` calls are present in the DUT, the optimiser will remove the pipe, since is does not recognise any usage for the pipe. Thus, ```PROFCOUNTER_FINISH()``` is the only macro that still uses ```write_pipe()``` directly. It is essential therefore to add this call at the end of your DUT, otherwise profiling won't work and the ProfCounter kernel will never end (i.e. ```clFinish()``` in the host will never return)!

## Limitations

* NDRange kernels are untested;
* With the placeholder approach, we have not experienced any problem regarding the profiler call being moved away from its original position, leading to incorrect cycle count, nor any changes in the final latency caused by ProfCounter's presence;
* If you have loops on your code that Vivado cannot pipeline (even when explicitely requested), with ProfCounter these loops can become pipelineable. Therefore in some cases Vivado might automatically pipeline those loops, affecting the hardware's latency. This is not expected behaviour and requires further study. Thus, all the projects in this repo have pipelining disabled.

## ProfCounter Commands

ProfCounter works based on commands received via an OpenCL pipe. The use of ```include/profcounter.h``` is recommended as it already declares the OpenCL pipe and also defines useful functions:

* ***PROFCOUNTER_INIT():*** initialise the placeholder. It must be called before any ```PROFCOUNTER_*()``` calls;
* ***PROFCOUNTER_CHECKPOINT_id():*** send a checkpoint command to ProfCounter. This is similar to ```PROFCOUNTER_STAMP()```, but also a checkpoint ID is saved with the timestamp:
	* The four most significant bits holds the checkpoint id, which is the ```id``` argument of this call. For current implementation, ```id``` can range from 0 to 11;
	* The remaining 60 bits holds the timestamp. Please note that using checkpoints therefore removes 4 bits of timestamping capability;
* ***PROFCOUNTER_STAMP():*** send a stamp command to ProfCounter. The current clock cycle is enqueued for storing on global memory;
* ***PROFCOUNTER_HOLD():*** stamp/checkpoint commands enqueued for write on global memory are held until ```PROFCOUNTER_FINISH()``` is called. This prevents ProfCounter from using the global memory bandwidth and possibly affecting performance of the kernels being tested;
* ***PROFCOUNTER_FINISH():*** finish execution of ProfCounter. This must be called at the end of your kernel being tested. If ```PROFCOUNTER_HOLD()``` was previously called, this call will flush the request FIFO to global memory before finishing. This call guarantees that ProfCounter will finish and it is essential for the OpenCL pipe to not be optimised away.

Stamp/checkpoint commands are enqueued in a request FIFO. In current implementation, requests are dropped if the FIFO gets full. There are two cases where this might happen:
* ```PROFCOUNTER_STAMP()```/```PROFCOUNTER_CHECKPOINT_X()``` is called several times in a small period of time, faster than the write of timestamps to global memory;
* ```PROFCOUNTER_HOLD()``` was called and ```PROFCOUNTER_STAMP()```/```PROFCOUNTER_CHECKPOINT_X()``` is used several times, filling up the request FIFO.

If you think that the request FIFO is not big enough for your implementation, you can increase its size by changing the first parameter in the FIFO declaration on file ```src/profCounter/SequentialWriter.v```, which is currently set to 256 words:
```
	/* ... */

	/* Request FIFO */
	FIFO#(256, 64) fifo(
		/* ... */
	);

	/* ... */
```

Please note that there is no way to differentiate in the global memory log between a stamp and a checkpoint command. For example, if the word ```0x2000000000000100``` is found on the global memory log, this can either indicate that a stamp command was issued at cycle ```0x2000000000000100``` or that a checkpoint command was issued at cycle ```0x100``` with ID ```0x2```. Avoid mixing stamp commands with checkpoint commands, or perform it in a way where the results can be interpreted without ambiguity.

## Make Options

You can specify a different platform and clock to the build as follows:
```
$ make PLATFORM=<PLATFORM> CLKID=<CLKID>
```
by default, ```PLATFORM=zcu102 CLKID=0```. You can use different clocks from your board by specifying ```CLKID```. For non-Zynq boards, modifications might be necessary in the ```Makefile```.

You can also compile just specific files:
```
$ make hw (complete synthesis and SD card files generation)
$ make host (compile host code only. It is not copied to the SD card generated folder)
$ make xclbin (synthesise the OpenCL kernel program)
$ make xo (compile the OpenCL objects)
$ make clean (clean your whole project)
```

The ```directives.tcl``` script makes use of an environment variable called ```PROFCOUNTERSRCROOT```, which points to the ```profCounter``` folder where all its sources and scripts are located. The provided makefiles in this project already handles this variable, however when adapting your code, make sure that this variable is set before calling the Xilinx toolchain!

## Files description

* ***base/src/***;
	* ***profCounter/FIFO/tb/:*** testbench for the FIFO module;
	* ***profCounter/FIFO/:*** simple FIFO implementation;
	* ***profCounter/generateXO.tcl:*** TCL script used during Vivado generation of the ```profCounter``` kernel;
	* ***profCounter/directives.tcl:*** TCL script called by Vivado to convert the placeholder calls to actual OpenCL pipe writes (see ***Scheduling Issues***) and performs final HLS scheduling and binding;
	* ***profCounter/transform.sh:*** transformation script: swaps placeholder calls by actual OpenCL pipe writes;
	* ***profCounter/BasicController.v:*** basic controller that complies with the RTL kernel specification from Xilinx SDx (see https://www.xilinx.com/html_docs/xilinx2018_3/sdaccel_doc/creating-rtl-kernels-qnk1504034323350.html#qbh1504034323531);
	* ***profCounter/commands.vh:*** macros defining the commands supported by ProfCounter;
	* ***profCounter/CommandUnit.v:*** translates the commands coming from the OpenCL pipe;
	* ***profCounter/profCounter.v:*** the kernel main module;
	* ***profCounter/SequentialWriter.v:*** simple AXI4 Master module for writing the timestamps on the global memory;
	* ***profCounter/Timestamper.v:*** simple cycle counter;
	* ***host.fpga.c:*** example host OpenCL code;
	* ***probe.cl:*** example DUT kernel;
	* ***profCounter.xml:*** XML description file for the ```profCounter``` kernel (see https://www.xilinx.com/html_docs/xilinx2018_3/sdaccel_doc/creating-rtl-kernels-qnk1504034323350.html#rzv1504034325561);
* ***example/***;
	* ***prof/:*** adapted BFS kernel from Rodinia with ProfCounter timestamping;
	* ***noprof/:*** adapted BFS kernel from Rodinia with no ProfCounter;

## TODOs

This is a work under construction. There are still some stuff to be done:

* Add support for NDRange kernels;
* Currently, timestamp requests are enqueued in a FIFO for global memory write. If this FIFO is full, further requests are dropped. It would be nice to implement some logic to avoid dropping OR notifying the user that timestamps were dropped;
* The ```SequentialWriter``` module is extremely simple, performing non-pipelined sequential writes of 64-bit words. It should be improved to perform pipelined burst writes and make use of the FIFOs from the AXI4 Slave interface;
* Guarantee that the placeholder calls will not affect scheduling in any case;
* Further study on the effects of automatic pipelining of non-pipelineable loops when ProfCounter is inserted.

## Acknowledgements

The project author would like to thank São Paulo Research Foundation (FAPESP), who funds the research where this project is inserted (Grant 2016/18937-7).

The opinions, hypotheses, conclusions or recommendations contained in this material are the sole responsibility of the author(s) and do not necessarily reflect FAPESP opinion.
