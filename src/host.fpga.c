/* ********************************************************************************************* */
/* * C Template for Kernel Execution                                                           * */
/* * Author: André Bannwart Perina                                                             * */
/* ********************************************************************************************* */
/* * Copyright (c) 2017 André B. Perina                                                        * */
/* *                                                                                           * */
/* * Permission is hereby granted, free of charge, to any person obtaining a copy of this      * */
/* * software and associated documentation files (the "Software"), to deal in the Software     * */
/* * without restriction, including without limitation the rights to use, copy, modify,        * */
/* * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to        * */
/* * permit persons to whom the Software is furnished to do so, subject to the following       * */
/* * conditions:                                                                               * */
/* *                                                                                           * */
/* * The above copyright notice and this permission notice shall be included in all copies     * */
/* * or substantial portions of the Software.                                                  * */
/* *                                                                                           * */
/* * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,       * */
/* * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR  * */
/* * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE * */
/* * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      * */
/* * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER    * */
/* * DEALINGS IN THE SOFTWARE.                                                                 * */
/* ********************************************************************************************* */

#include <CL/opencl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "common.h"

/**
 * @brief Standard statements for function error handling and printing.
 *
 * @param funcName Function name that failed.
 */
#define FUNCTION_ERROR_STATEMENTS(funcName) {\
	rv = EXIT_FAILURE;\
	PRINT_FAIL();\
	fprintf(stderr, "Error: %s failed with return code %d.\n", funcName, fRet);\
}

/**
 * @brief Standard statements for POSIX error handling and printing.
 *
 * @param arg Arbitrary string to the printed at the end of error string.
 */
#define POSIX_ERROR_STATEMENTS(arg) {\
	rv = EXIT_FAILURE;\
	PRINT_FAIL();\
	fprintf(stderr, "Error: %s: %s\n", strerror(errno), arg);\
}

int main(int argc, char *argv[]) {
	/* Return variable */
	int rv = EXIT_SUCCESS;

	/* OpenCL and aux variables */
	int i = 0;
	cl_uint platformsLen, devicesLen;
	cl_int fRet;
	cl_platform_id *platforms = NULL;
	cl_device_id *devices = NULL;
	cl_context context = NULL;
	cl_command_queue queueProfCounter = NULL;
	cl_command_queue queueProbe = NULL;
	FILE *programFile = NULL;
	size_t programSz;
	char *programContent = NULL;
	cl_int programRet;
	cl_program program = NULL;
	cl_kernel kernelProfCounter = NULL;
	cl_kernel kernelProbe = NULL;
	bool loopFlag = false;
	long totalTime;
	struct timeval tThen, tNow, tDelta, tExecTime;
	timerclear(&tExecTime);
	cl_uint workDimProfCounter = 1;
	size_t globalSizeProfCounter[1] = {
		1
	};
	size_t localSizeProfCounter[1] = {
		1
	};
	cl_uint workDimProbe = 1;
	size_t globalSizeProbe[1] = {
		1
	};
	size_t localSizeProbe[1] = {
		1
	};

	/* Input/output variables */
	long *log = calloc(65536, sizeof(long));
	cl_mem logK = NULL;
	unsigned *timeline = malloc(10 * sizeof(unsigned));
	cl_mem timelineK = NULL;
	char mustHold = 0;

	/* Populate timeline */
	unsigned timelineFixed[10] = {0, 15, 30, 40, 50, 70, 100, 120, 199, 200};
	for(i = 0; i < 10; i++)
		timeline[i] = timelineFixed[i];
	i = 0;

	/* Update mustHold variable if command-line argument was provided */
	if(argc > 1 && !strcmp(argv[1], "hold"))
		mustHold = 1;

	/* Get platforms IDs */
	PRINT_STEP("Getting platforms IDs...");
	fRet = clGetPlatformIDs(0, NULL, &platformsLen);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clGetPlatformIDs"));
	platforms = malloc(platformsLen * sizeof(cl_platform_id));
	fRet = clGetPlatformIDs(platformsLen, platforms, NULL);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clGetPlatformIDs"));
	PRINT_SUCCESS();

	/* Get devices IDs for first platform availble */
	PRINT_STEP("Getting devices IDs for first platform...");
	fRet = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &devicesLen);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clGetDevicesIDs"));
	devices = malloc(devicesLen * sizeof(cl_device_id));
	fRet = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, devicesLen, devices, NULL);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clGetDevicesIDs"));
	PRINT_SUCCESS();

	/* Create context for first available device */
	PRINT_STEP("Creating context...");
	context = clCreateContext(NULL, 1, devices, NULL, NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateContext"));
	PRINT_SUCCESS();

	/* Create command queue for profCounter kernel */
	PRINT_STEP("Creating command queue for \"profCounter\"...");
	queueProfCounter = clCreateCommandQueue(context, devices[0], 0, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateCommandQueue"));
	PRINT_SUCCESS();

	/* Create command queue for probe kernel */
	PRINT_STEP("Creating command queue for \"probe\"...");
	queueProbe = clCreateCommandQueue(context, devices[0], 0, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateCommandQueue"));
	PRINT_SUCCESS();

	/* Open binary file */
	PRINT_STEP("Opening program binary...");
	programFile = fopen("program.xclbin", "rb");
	ASSERT_CALL(programFile, POSIX_ERROR_STATEMENTS("program.xclbin"));
	PRINT_SUCCESS();

	/* Get size and read file */
	PRINT_STEP("Reading program binary...");
	fseek(programFile, 0, SEEK_END);
	programSz = ftell(programFile);
	fseek(programFile, 0, SEEK_SET);
	programContent = malloc(programSz);
	fread(programContent, programSz, 1, programFile);
	fclose(programFile);
	programFile = NULL;
	PRINT_SUCCESS();

	/* Create program from binary file */
	PRINT_STEP("Creating program from binary...");
	program = clCreateProgramWithBinary(context, 1, devices, &programSz, (const unsigned char **) &programContent, &programRet, &fRet);
	ASSERT_CALL(CL_SUCCESS == programRet, FUNCTION_ERROR_STATEMENTS("clCreateProgramWithBinary (when loading binary)"));
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateProgramWithBinary"));
	PRINT_SUCCESS();

	/* Build program */
	PRINT_STEP("Building program...");
	fRet = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clBuildProgram"));
	PRINT_SUCCESS();

	/* Create profCounter kernel */
	PRINT_STEP("Creating kernel \"profCounter\" from program...");
	kernelProfCounter = clCreateKernel(program, "profCounter", &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateKernel"));
	PRINT_SUCCESS();

	/* Create probe kernel */
	PRINT_STEP("Creating kernel \"probe\" from program...");
	kernelProbe = clCreateKernel(program, "probe", &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateKernel"));
	PRINT_SUCCESS();

	/* Create input and output buffers */
	PRINT_STEP("Creating buffers...");
	logK = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 65536 * sizeof(long), NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateBuffer (logK)"));
	timelineK = clCreateBuffer(context, CL_MEM_READ_ONLY, 10 * sizeof(unsigned), NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateBuffer (timelineK)"));
	PRINT_SUCCESS();

	/* Set kernel arguments for profCounter */
	PRINT_STEP("Setting kernel arguments for \"profCounter\"...");
	fRet = clSetKernelArg(kernelProfCounter, 0, sizeof(cl_mem), &logK);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (logK)"));
	PRINT_SUCCESS();

	/* Set kernel arguments for probe */
	PRINT_STEP("Setting kernel arguments for \"probe\"...");
	fRet = clSetKernelArg(kernelProbe, 0, sizeof(cl_mem), &timelineK);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (timelineK)"));
	fRet = clSetKernelArg(kernelProbe, 1, sizeof(char), &mustHold);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (mustHold)"));
	PRINT_SUCCESS();

	do {
		/* Setting input and output buffers */
		PRINT_STEP("[%d] Setting buffers...", i);
		fRet = clEnqueueWriteBuffer(queueProfCounter, logK, CL_TRUE, 0, 65536 * sizeof(long), log, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueWriteBuffer (logK)"));
		fRet = clEnqueueWriteBuffer(queueProbe, timelineK, CL_TRUE, 0, 10 * sizeof(unsigned), timeline, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueWriteBuffer (timelineK)"));
		PRINT_SUCCESS();

		PRINT_STEP("[%d] Running kernels...", i);
		fRet = clEnqueueNDRangeKernel(queueProfCounter, kernelProfCounter, workDimProfCounter, NULL, globalSizeProfCounter, localSizeProfCounter, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueNDRangeKernel"));

		// XXX: wait to settle down
		sleep(1);

		gettimeofday(&tThen, NULL);
		fRet = clEnqueueNDRangeKernel(queueProbe, kernelProbe, workDimProbe, NULL, globalSizeProbe, localSizeProbe, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueNDRangeKernel"));
		clFinish(queueProbe);
		gettimeofday(&tNow, NULL);
		clFinish(queueProfCounter);
		PRINT_SUCCESS();

		/* Get output buffers */
		PRINT_STEP("[%d] Getting kernels arguments...", i);
		fRet = clEnqueueReadBuffer(queueProfCounter, logK, CL_TRUE, 0, 65536 * sizeof(long), log, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueReadBuffer"));
		PRINT_SUCCESS();

		timersub(&tNow, &tThen, &tDelta);
		timeradd(&tExecTime, &tDelta, &tExecTime);
		i++;
	} while(loopFlag);

	/* Print profiling results */
	totalTime = (1000000 * tExecTime.tv_sec) + tExecTime.tv_usec;
	printf("Elapsed time spent on kernels: %ld us; Average time per iteration: %lf us.\n", totalTime, totalTime / (double) i);

	printf("Received values (assuming II of 136 cycles):\n");
	printf("|    | Absolute values                       || II-normalised values                  |\n");
	printf("|  i |       t(i) |  t(i)-t(0) | t(i)-t(i-1) ||       t(i) |  t(i)-t(0) | t(i)-t(i-1) |\n");
	for(i = 0; i < 50; i++) {
		if(!(log[i]))
			break;

		printf(
			"| %2d | %10ld | %10ld |  %10ld || %10ld | %10ld |  %10ld |\n", i,
			log[i], log[i] - log[0], i? (log[i] - log[i-1]) : 0,
			log[i] / 136, (log[i] - log[0]) / 136, i? (log[i] - log[i-1]) / 136 : 0
		);
	}

_err:

	/* Dealloc buffers */
	if(logK)
		clReleaseMemObject(logK);
	if(timelineK)
		clReleaseMemObject(timelineK);

	/* Dealloc variables */
	free(log);
	free(timeline);

	/* Dealloc kernels */
	if(kernelProfCounter)
		clReleaseKernel(kernelProfCounter);
	if(kernelProbe)
		clReleaseKernel(kernelProbe);

	/* Dealloc program */
	if(program)
		clReleaseProgram(program);
	if(programContent)
		free(programContent);
	if(programFile)
		fclose(programFile);

	/* Dealloc queues */
	if(queueProfCounter)
		clReleaseCommandQueue(queueProfCounter);
	if(queueProbe)
		clReleaseCommandQueue(queueProbe);

	/* Last OpenCL variables */
	if(context)
		clReleaseContext(context);
	if(devices)
		free(devices);
	if(platforms)
		free(platforms);

	return rv;
}
