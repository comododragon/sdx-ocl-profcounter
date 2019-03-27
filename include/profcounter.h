#ifndef PROFCOUNTER_H
#define PROFCOUNTER_H

/* Source-end of AXI4-Stream that goes to profCounter */
__write_only pipe unsigned p0 __attribute__((xcl_reqd_pipe_depth(16)));

#define PROFCOUNTER_INIT() __private const unsigned __PROFCOUNTER_COMMAND_WRITE__ = 0x1, __PROFCOUNTER_COMMAND_FINISH__ = 0x2

#define PROFCOUNTER_STAMP() write_pipe(p0, &__PROFCOUNTER_COMMAND_WRITE__)

#define PROFCOUNTER_FINISH() write_pipe(p0, &__PROFCOUNTER_COMMAND_FINISH__)

#endif
