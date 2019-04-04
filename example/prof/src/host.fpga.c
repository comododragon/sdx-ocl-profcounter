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
#include "prepostambles.h"

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
	cl_command_queue queueBfs = NULL;
	FILE *programFile = NULL;
	size_t programSz;
	char *programContent = NULL;
	cl_int programRet;
	cl_program program = NULL;
	cl_kernel kernelProfCounter = NULL;
	cl_kernel kernelBfs = NULL;
	bool loopFlag = false;
	bool invalidDataFound = false;
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
	cl_uint workDimBfs = 1;
	size_t globalSizeBfs[1] = {
		1
	};
	size_t localSizeBfs[1] = {
		1
	};

	/* Input/output variables */
	long *log = calloc(65536, sizeof(long));
	cl_mem logK = NULL;
	unsigned int *levels = malloc(1000 * sizeof(unsigned int));
	unsigned int *levelsC = malloc(1000 * sizeof(unsigned int));
	cl_mem levelsK = NULL;
	unsigned int *edgeOffsets = malloc(1001 * sizeof(unsigned int));
	cl_mem edgeOffsetsK = NULL;
	unsigned int *edgeList = malloc(1998 * sizeof(unsigned int));
	cl_mem edgeListK = NULL;
	unsigned int numVertices;

	/* Calling preamble function */
	PRINT_STEP("Calling preamble function...");
	PREAMBLE(levels, 1000, levelsC, 1000, edgeOffsets, 1001, edgeList, 1998, numVertices);
	PRINT_SUCCESS();

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

	/* Create command queue for bfs kernel */
	PRINT_STEP("Creating command queue for \"bfs\"...");
	queueBfs = clCreateCommandQueue(context, devices[0], 0, &fRet);
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

	/* Create bfs kernel */
	PRINT_STEP("Creating kernel \"bfs\" from program...");
	kernelBfs = clCreateKernel(program, "bfs", &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateKernel"));
	PRINT_SUCCESS();

	/* Create input and output buffers */
	PRINT_STEP("Creating buffers...");
	logK = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 65536 * sizeof(long), NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateBuffer (logK)"));
	levelsK = clCreateBuffer(context, CL_MEM_READ_WRITE, 1000 * sizeof(unsigned int), NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateBuffer (levelsK)"));
	edgeOffsetsK = clCreateBuffer(context, CL_MEM_READ_ONLY, 1001 * sizeof(unsigned int), NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateBuffer (edgeOffsetsK)"));
	edgeListK = clCreateBuffer(context, CL_MEM_READ_ONLY, 1998 * sizeof(unsigned int), NULL, &fRet);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clCreateBuffer (edgeListK)"));
	PRINT_SUCCESS();

	/* Set kernel arguments for profCounter */
	PRINT_STEP("Setting kernel arguments for \"profCounter\"...");
	fRet = clSetKernelArg(kernelProfCounter, 0, sizeof(cl_mem), &logK);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (logK)"));
	PRINT_SUCCESS();

	/* Set kernel arguments for bfs */
	PRINT_STEP("Setting kernel arguments for \"bfs\"...");
	fRet = clSetKernelArg(kernelBfs, 0, sizeof(cl_mem), &levelsK);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (levelsK)"));
	fRet = clSetKernelArg(kernelBfs, 1, sizeof(cl_mem), &edgeOffsetsK);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (edgeOffsetsK)"));
	fRet = clSetKernelArg(kernelBfs, 2, sizeof(cl_mem), &edgeListK);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (edgeListK)"));
	fRet = clSetKernelArg(kernelBfs, 3, sizeof(unsigned int), &numVertices);
	ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clSetKernelArg (numVertices)"));
	PRINT_SUCCESS();

	do {
		/* Setting input and output buffers */
		PRINT_STEP("[%d] Setting buffers...", i);
		fRet = clEnqueueWriteBuffer(queueProfCounter, logK, CL_TRUE, 0, 65536 * sizeof(long), log, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueWriteBuffer (logK)"));
		fRet = clEnqueueWriteBuffer(queueBfs, levelsK, CL_TRUE, 0, 1000 * sizeof(unsigned int), levels, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueWriteBuffer (levelsK)"));
		fRet = clEnqueueWriteBuffer(queueBfs, edgeOffsetsK, CL_TRUE, 0, 1001 * sizeof(unsigned int), edgeOffsets, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueWriteBuffer (edgeOffsetsK)"));
		fRet = clEnqueueWriteBuffer(queueBfs, edgeListK, CL_TRUE, 0, 1998 * sizeof(unsigned int), edgeList, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueWriteBuffer (edgeListK)"));
		PRINT_SUCCESS();

		PRINT_STEP("[%d] Running kernels...", i);
		fRet = clEnqueueNDRangeKernel(queueProfCounter, kernelProfCounter, workDimProfCounter, NULL, globalSizeProfCounter, localSizeProfCounter, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueNDRangeKernel"));

		// XXX: wait to settle down
		sleep(1);

		gettimeofday(&tThen, NULL);
		fRet = clEnqueueNDRangeKernel(queueBfs, kernelBfs, workDimBfs, NULL, globalSizeBfs, localSizeBfs, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueNDRangeKernel"));
		clFinish(queueBfs);
		gettimeofday(&tNow, NULL);
		clFinish(queueProfCounter);
		PRINT_SUCCESS();

		/* Get output buffers */
		PRINT_STEP("[%d] Getting kernels arguments...", i);
		fRet = clEnqueueReadBuffer(queueProfCounter, logK, CL_TRUE, 0, 65536 * sizeof(long), log, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueReadBuffer"));
		fRet = clEnqueueReadBuffer(queueBfs, levelsK, CL_TRUE, 0, 1000 * sizeof(unsigned int), levels, 0, NULL, NULL);
		ASSERT_CALL(CL_SUCCESS == fRet, FUNCTION_ERROR_STATEMENTS("clEnqueueReadBuffer"));
		PRINT_SUCCESS();

		timersub(&tNow, &tThen, &tDelta);
		timeradd(&tExecTime, &tDelta, &tExecTime);
		i++;
	} while(loopFlag);

	/* Print profiling results */
	totalTime = (1000000 * tExecTime.tv_sec) + tExecTime.tv_usec;
	printf("Elapsed time spent on kernels: %ld us; Average time per iteration: %lf us.\n", totalTime, totalTime / (double) i);

	/* Validate received data */
	PRINT_STEP("Validating received data...");
	for(i = 0; i < 1000; i++) {
		if(levelsC[i] != levels[i]) {
			if(!invalidDataFound) {
				PRINT_FAIL();
				invalidDataFound = true;
			}
			printf("Variable levels[%d]: expected %u got %u.\n", i, levelsC[i], levels[i]);
		}
	}
	if(!invalidDataFound)
		PRINT_SUCCESS();

	printf("Information provided by \"profCounter\":\n");
	printf("|          |        Timestamp        |\n");
	printf("| Chkpt ID |   Absolute |   Relative |\n");
	for(i = 0; i < 65536; i++) {
		if(!(log[i]))
			break;

		unsigned checkpointID = (log[i] >> 60) & 0xF;
		uint64_t timestamp0 = log[0] & 0xFFFFFFFFFFFFFFF;
		uint64_t timestampi = log[i] & 0xFFFFFFFFFFFFFFF;

		printf("|       %2x | %10ld | %10ld |\n", checkpointID, timestampi, timestampi - timestamp0);
	}

_err:

	/* Dealloc buffers */
	if(logK)
		clReleaseMemObject(logK);
	if(levelsK)
		clReleaseMemObject(levelsK);
	if(edgeOffsetsK)
		clReleaseMemObject(edgeOffsetsK);
	if(edgeListK)
		clReleaseMemObject(edgeListK);

	/* Dealloc variables */
	free(log);
	free(levels);
	free(levelsC);
	free(edgeOffsets);
	free(edgeList);

	/* Dealloc kernels */
	if(kernelProfCounter)
		clReleaseKernel(kernelProfCounter);
	if(kernelBfs)
		clReleaseKernel(kernelBfs);

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
	if(queueBfs)
		clReleaseCommandQueue(queueBfs);

	/* Last OpenCL variables */
	if(context)
		clReleaseContext(context);
	if(devices)
		free(devices);
	if(platforms)
		free(platforms);

	return rv;
}
