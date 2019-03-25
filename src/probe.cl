/* Source-end of AXI4-Stream that goes to profCounter */
__write_only pipe unsigned p0 __attribute__((xcl_reqd_pipe_depth(16)));

__kernel void probe(__global unsigned * restrict timeline) {
	int i, j;

	/* Pipe content to be written must be addressable */
	__private const unsigned WRITE = 0x1;
	__private const unsigned FINISH = 0x2;

	/* Generate 10 timestamps, each on a given "epoch" given by timeline */
	for(i = 0, j = 0; i < 10; j++) {
		/* Send command to save timestamp */
		if(timeline[i] == j) {
			write_pipe(p0, &WRITE);
			i++;
		}
	}

	/* Send command to shut down profCounter */
	write_pipe(p0, &FINISH);
}
