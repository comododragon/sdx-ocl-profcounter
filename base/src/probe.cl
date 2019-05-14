#include "profcounter.h"

__attribute__((reqd_work_group_size(1,1,1)))
__kernel void probe(__global unsigned * restrict timeline, char mustHold) {
	int i, j;

	/* Initialise timestamper */
	PROFCOUNTER_INIT();

	if(mustHold) {
		/* Request values to be held; they are written only after PROFCOUNTER_FINISH() is called */
		PROFCOUNTER_HOLD();
	}

	/* Generate a timestamp without checkpoint ID */
	PROFCOUNTER_STAMP();

	/* Generate 10 timestamps, each on a given "epoch" given by timeline */
	for(i = 0, j = 0; i < 10; j++) {
		if(timeline[i] == j) {
			/* Send command to save timestamp with checkpoint ID 10 */
			PROFCOUNTER_CHECKPOINT_10();
			i++;
		}
	}

	/* Generate a timestamp with checkpoint ID 11 */
	PROFCOUNTER_CHECKPOINT_11();

	/* Send command to shut down profCounter */
	PROFCOUNTER_FINISH();
}
