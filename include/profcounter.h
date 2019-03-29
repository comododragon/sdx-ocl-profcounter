#ifndef PROFCOUNTER_H
#define PROFCOUNTER_H

/* Source-end of AXI4-Stream that goes to profCounter */
__write_only pipe unsigned p0 __attribute__((xcl_reqd_pipe_depth(16)));

#define PROFCOUNTER_INIT() __private const unsigned __PROFCOUNTER_COMM_STAMP__ = 0x1, __PROFCOUNTER_COMM_HOLD__ = 0x2,__PROFCOUNTER_COMM_FINISH__ = 0x3

#define PROFCOUNTER_STAMP() write_pipe(p0, &__PROFCOUNTER_COMM_STAMP__)

#define PROFCOUNTER_HOLD() write_pipe(p0, &__PROFCOUNTER_COMM_HOLD__)

#define PROFCOUNTER_FINISH() write_pipe(p0, &__PROFCOUNTER_COMM_FINISH__)

#endif
