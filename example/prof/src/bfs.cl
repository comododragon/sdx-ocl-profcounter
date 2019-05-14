#include "profcounter.h"

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics: enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics: enable

#define WORKGROUP_SZ 8192
#define WARP_SZ 32
#define CHUNK_SZ 32

__attribute__((reqd_work_group_size(1,1,1)))
__kernel void bfs(__global unsigned * restrict levels, __global unsigned * restrict edgeOffsets, __global unsigned * restrict edgeList, unsigned numVertices) {
	bool flag = true;

	PROFCOUNTER_INIT();
	PROFCOUNTER_HOLD();

	PROFCOUNTER_CHECKPOINT_0();

	/* Original host loop */
	for(int curr = 0; flag; curr++) {
		flag = false;

		PROFCOUNTER_CHECKPOINT_1();

		/* Conversion from NDRange do task */
		for(int tid = 0; tid < WORKGROUP_SZ; tid++) {
			int offset = tid % WARP_SZ;
			int id = tid / WARP_SZ;
			int v1 = id * CHUNK_SZ;
			int chunkSz = CHUNK_SZ + 1;

			if((v1 + chunkSz) >= numVertices) {
				chunkSz = numVertices - v1 + 1;

				if(chunkSz < 0)
					chunkSz = 0;
			}

			for(int v = v1; v < (chunkSz - 1 + v1); v++) {
				if(curr == levels[v]) {
					unsigned numNbr = edgeOffsets[v + 1] - edgeOffsets[v];
					unsigned nbrOff = edgeOffsets[v];

					for(int i = offset; i < numNbr; i += WARP_SZ) {
						int v = edgeList[i + nbrOff];

						if(UINT_MAX == levels[v]) {
							levels[v] = curr + 1;
							flag = true;
						}
					}
				}
			}
		}

		PROFCOUNTER_CHECKPOINT_2();
	}

	PROFCOUNTER_CHECKPOINT_3();

	PROFCOUNTER_FINISH();
}
