#include "profcounter.h"

__kernel void probe(__global unsigned * restrict timeline, char mustHold) {
	int i, j;

	/* Initialise timestamper */
	PROFCOUNTER_INIT();

	if(mustHold) {
		/* Request values to be held; they are written only after PROFCOUNTER_FINISH() is called */
		PROFCOUNTER_HOLD();
	}

	/* Generate 10 timestamps, each on a given "epoch" given by timeline */
	for(i = 0, j = 0; i < 10; j++) {
		if(timeline[i] == j) {
			/* Send command to save timestamp */
			PROFCOUNTER_STAMP();
			i++;
		}
	}

	/* Send command to shut down profCounter */
	PROFCOUNTER_FINISH();
}
