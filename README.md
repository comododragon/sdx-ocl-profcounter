# SDx OpenCL ProfCounter

OpenCL Fine-grain Profiling Kernel for Xilinx SDx Development Environment

## Authors

* André Bannwart Perina

## Introduction

The ProfCount is a framework developed for fine-grain profiling of OpenCL kernels developed using the Xilinx SDx environment. It consists of:

* A header that may be included to your design under test (DUT, namely an OpenCL kernel), including functions for timestamping;
* An RTL kernel that performs cycle count.

Using the timestamping function, one can precisely count the cycles of a specific region of an OpenCL kernel, as the RTL kernel is driven by the same clock as the DUT kernel.

## Licence

Most of this project is distributed under the BSD-3-Clause licence. See LICENSE.TXT for details.

The ```include/common.h``` header is distributed under the GPL-3.0 licence. See the following repo for details: https://github.com/comododragon/commonh

## Usage Introduction

This repository provides a functional example project of ProfCounter. It consists of two kernels:

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

* ***src/profCounter/:*** the RTL kernel that performs all the magic. This is the pseudo-code for the kernel:
```
__read_only pipe unsigned p0 __attribute__((xcl_reqd_pipe_depth(16)));

__kernel void profCounter(__global long * restrict log) {
	long *offset = 0x0;
	unsigned command = 0x0;

	for(long cycleCount = 0; command != 0x2; cycleCount++) {
		read_pipe(p0, &command);

		if(0x1 == command)
			log[offset++] = cycleCount;
	}
}
```

This pseudo-kernel is just for illustration purposes. As of Xilinx SDx 2018.2, non-blocking ```read_pipe``` is not implemented, therefore the cycle count is infeasible to be performed accurately, thus the kernel was implemented in Verilog following the same logic as presented above, though pipe reading and timestamping are performed in parallel and non-blocking. Thus, the behaviour if this kernel is:

* It starts cycle counting as soon as the kernel is launched using ```clEnqueueTask```;
* It saves a timestamp on the global memory space when a ```0x1``` command is received through the ```p0``` pipe;
* It stops counting when a ```0x2``` command is received through the ```p0```pipe.

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

You must ensure that on the host code, the ```profCounter``` gets started BEFORE ```probe```. The ```include/profcounter.h``` is a very simple header that already provides the pipe declaration for your DUT and macros that writes commands to the pipe. You must also ensure that the ```0x2``` command is sent to the profiler (using ```PROFCOUNTER_FINISH()``` for convenience), otherwise the profiling kernel will execute indefinitely and likely to hang your host code execution.

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
* With this example code, timestamping will be requested when ```j``` is 0, 15, 30, 40, 50, 70, 100, 120, 199 and 200 on the ```probe``` kernel. Considering that this loop has an initiation interval (II) of 136 cycles, the following output is expected:
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
Elapsed time spent on kernels: 491 us; Average time per iteration: 491.000000 us.
Received values (assuming II of 136 cycles):
|    | Absolute values                       || II-normalised values                  |
|  i |       t(i) |  t(i)-t(0) | t(i)-t(i-1) ||       t(i) |  t(i)-t(0) | t(i)-t(i-1) |
|  0 |   75005518 |          0 |           0 ||     551511 |          0 |           0 |
|  1 |   75007558 |       2040 |        2040 ||     551526 |         15 |          15 |
|  2 |   75009598 |       4080 |        2040 ||     551541 |         30 |          15 |
|  3 |   75010958 |       5440 |        1360 ||     551551 |         40 |          10 |
|  4 |   75012318 |       6800 |        1360 ||     551561 |         50 |          10 |
|  5 |   75015038 |       9520 |        2720 ||     551581 |         70 |          20 |
|  6 |   75019118 |      13600 |        4080 ||     551611 |        100 |          30 |
|  7 |   75021838 |      16320 |        2720 ||     551631 |        120 |          20 |
|  8 |   75032582 |      27064 |       10744 ||     551710 |        199 |          79 |
|  9 |   75032718 |      27200 |         136 ||     551711 |        200 |           1 |
```
* ***Note:*** you can execute the host code with ```hold``` argument to activate the hold behaviour of ProfCounter (see Section ***ProfCounter Commands***):
```
$ ./execute hold
```

## ProfCounter Commands

ProfCounter works based on commands received via an OpenCL pipe. The usage of ```include/profcounter.h``` is recommended as it already declares the OpenCL pipe and also defines useful functions:

* ```PROFCOUNTER_INIT()```: initialise the private variables that contain the command identifiers;
* ```PROFCOUNTER_STAMP()```: send a stamp command to ProfCounter. The current clock cycle is enqueued for storing on global memory;
* ```PROFCOUNTER_HOLD()```: stamp commands enqueued for write on global memory are held until ```PROFCOUNTER_FINISH()``` is called. This prevents ProfCounter from using the global memory bandwidth and possibly affecting performance of the kernels being tested;
* ```PROFCOUNTER_FINISH()```: finish execution of ProfCounter. This must be called at the end of your kernel being tested. If ```PROFCOUNTER_HOLD()``` was previously called, this call will flush the request FIFO to global memory before finishing.

Stamp commands are enqueued in a request FIFO. In current implementation, stamp requests are dropped if the FIFO gets full. There are two cases where this might happen:
* ```PROFCOUNTER_STAMP()``` is called several times in a small period of time, faster than the write of timestamps to global memory;
* ```PROFCOUNTER_HOLD()``` was called and ```PROFCOUNTER_STAMP()``` several times, filling up the request FIFO.

If you think that the request FIFO is not big enough for your implementation, you can increase its size by changing the first parameter in the FIFO declaration on file ```srd/profCounter/SequentialWriter.v```, which is currently set to 256 words:
```
	/* ... */

	/* Request FIFO */
	FIFO#(256, 64) fifo(
		/* ... */
	);

	/* ... */
```

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

## Files description

* ***src***;
	* ***profCounter/FIFO/tb/:*** testbench for the FIFO module;
	* ***profCounter/FIFO/:*** simple FIFO implementation;
	* ***profCounter/generateXO.tcl:*** TCL script used during Vivado generation of the ```profCounter``` kernel;
	* ***profCounter/BasicController.v:*** basic controller that complies with the RTL kernel specification from Xilinx SDx (see https://www.xilinx.com/html_docs/xilinx2018_3/sdaccel_doc/creating-rtl-kernels-qnk1504034323350.html#qbh1504034323531);
	* ***profCounter/commands.vh:*** macros defining the commands supported by ProfCounter;
	* ***profCounter/CommandUnit.v:*** translates the commands coming from the OpenCL pipe;
	* ***profCounter/profCounter.v:*** the kernel main module;
	* ***profCounter/SequentialWriter.v:*** simple AXI4 Master module for writing the timestamps on the global memory;
	* ***profCounter/Timestamper.v:*** simple cycle counter;
	* ***host.fpga.c:*** example host OpenCL code;
	* ***probe.cl:*** example DUT kernel;
	* ***profCounter.xml:*** XML description file for the ```profCounter``` kernel (see https://www.xilinx.com/html_docs/xilinx2018_3/sdaccel_doc/creating-rtl-kernels-qnk1504034323350.html#rzv1504034325561).

## TODOs

This is a work under construction. There are still some stuff to be done:

* Add support for NDRange kernels;
* Currently, timestamp requests are enqueued in a FIFO for global memory write. If this FIFO is full, further requests are dropped. It would be nice to implement some logic to avoid dropping OR notifying the used that timestamps were dropped;
* The ```SequentialWriter``` module is extremely simple, performing non-pipelined sequential writes of 64-bit words. It should be improved to perform pipelined burst writes and make use of the FIFOs from the AXI4 Slave interface.

## Acknowledgements

The project author would like to thank São Paulo Research Foundation (FAPESP), who funds the research where this project is inserted (Grant 2016/18937-7).
