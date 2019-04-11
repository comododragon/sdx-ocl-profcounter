#ifndef PROFCOUNTER_H
#define PROFCOUNTER_H

/* Source-end of AXI4-Stream that goes to profCounter */
__write_only pipe unsigned p0 __attribute__((xcl_reqd_pipe_depth(16)));

/* Initialise profcounter variables */
#define PROFCOUNTER_INIT() __private const unsigned\
	__PROFCOUNTER_COMM_CHECKPOINT_0__ = 0x1,\
	__PROFCOUNTER_COMM_CHECKPOINT_1__ = 0x2,\
	__PROFCOUNTER_COMM_CHECKPOINT_2__ = 0x3,\
	__PROFCOUNTER_COMM_CHECKPOINT_3__ = 0x4,\
	__PROFCOUNTER_COMM_CHECKPOINT_4__ = 0x5,\
	__PROFCOUNTER_COMM_CHECKPOINT_5__ = 0x6,\
	__PROFCOUNTER_COMM_CHECKPOINT_6__ = 0x7,\
	__PROFCOUNTER_COMM_CHECKPOINT_7__ = 0x8,\
	__PROFCOUNTER_COMM_CHECKPOINT_8__ = 0x9,\
	__PROFCOUNTER_COMM_CHECKPOINT_9__ = 0xA,\
	__PROFCOUNTER_COMM_CHECKPOINT_10__ = 0xB,\
	__PROFCOUNTER_COMM_CHECKPOINT_11__ = 0xC,\
	__PROFCOUNTER_COMM_STAMP__ = 0xD,\
	__PROFCOUNTER_COMM_HOLD__ = 0xE,\
	__PROFCOUNTER_COMM_FINISH__ = 0xF\

/* Issue a checkpoint command, with the ID defined at runtime */
#define PROFCOUNTER_CHECKPOINT(id) do {\
	__private unsigned __id__ = ((id) & 0xF) + 1;\
	if(__id__ > 0 && __id__ <= 12) {\
		write_pipe(p0, &__id__);\
	}\
} while(0)

/* Issue a checkpoint command, with the ID defined at compile-time */
#define PROFCOUNTER_CHECKPOINT_0() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_0__)
#define PROFCOUNTER_CHECKPOINT_1() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_1__)
#define PROFCOUNTER_CHECKPOINT_2() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_2__)
#define PROFCOUNTER_CHECKPOINT_3() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_3__)
#define PROFCOUNTER_CHECKPOINT_4() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_4__)
#define PROFCOUNTER_CHECKPOINT_5() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_5__)
#define PROFCOUNTER_CHECKPOINT_6() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_6__)
#define PROFCOUNTER_CHECKPOINT_7() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_7__)
#define PROFCOUNTER_CHECKPOINT_8() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_8__)
#define PROFCOUNTER_CHECKPOINT_9() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_9__)
#define PROFCOUNTER_CHECKPOINT_10() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_10__)
#define PROFCOUNTER_CHECKPOINT_11() write_pipe(p0, &__PROFCOUNTER_COMM_CHECKPOINT_11__)

/* Issue a stamp command */
#define PROFCOUNTER_STAMP() write_pipe(p0, &__PROFCOUNTER_COMM_STAMP__)

/* Request profcounter to hold all writes to global memory until PROFCOUNTER_FINISH() is called */
#define PROFCOUNTER_HOLD() write_pipe(p0, &__PROFCOUNTER_COMM_HOLD__)

/* Finish kernel execution and release hold if applicable */
#define PROFCOUNTER_FINISH() write_pipe(p0, &__PROFCOUNTER_COMM_FINISH__)

#endif