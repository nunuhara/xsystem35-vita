#ifndef __VITA_PORT__
#define __VITA_PORT__

#include <debugnet.h>
#undef NONE  // are
#undef INFO  // you
#undef ERROR // fucking
#undef DEBUG // serious
#define DEBUGLEVEL_NONE  0
#define DEBUGLEVEL_INFO  1
#define DEBUGLEVEL_ERROR 2
#define DEBUGLEVEL_DEBUG 3

#define PATH_MAX 1024 // probably OK?

#include <psp2/kernel/processmgr.h>

SceUInt64 process_time;

#define TIME_START() process_time = sceKernelGetProcessTimeWide()
#define TIME_END(fun)												\
	do {															\
		SceUInt64 diff = sceKernelGetProcessTimeWide() - process_time;	\
		NOTICE("%s ran in %llu\n", fun, diff);			\
	} while (0);

#endif /* __VITA_PORT__ */
