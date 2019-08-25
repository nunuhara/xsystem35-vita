#ifndef __VITA_PORT__
#define __VITA_PORT__

#ifdef DEBUG
#include <debugnet.h>
#undef NONE  // are
#undef INFO  // you
#undef ERROR // fucking
#undef DEBUG // serious
#define DEBUGLEVEL_NONE  0
#define DEBUGLEVEL_INFO  1
#define DEBUGLEVEL_ERROR 2
#define DEBUGLEVEL_DEBUG 3
#define DEBUG 1
#else
#define debugNetInit(addr, port, level)
#define debugNetPrintf(level, format, ...) printf(format, ##__VA_ARGS__)
#endif

#define PATH_MAX 2048 // probably OK?
#define VITA_HOME "ux0:/data/xsystem35"
#define VITA_W 960
#define VITA_H 544

#include <psp2/kernel/processmgr.h>

SceUInt64 _process_time;

#define TIME_START() _process_time = sceKernelGetProcessTimeWide()
#define TIME_END(fun)												\
	do {															\
		SceUInt64 diff = sceKernelGetProcessTimeWide() - _process_time;	\
		NOTICE("%s ran in %llu\n", fun, diff);			\
	} while (0);

int vita_launcher(int *argc, char ***argv);

#endif /* __VITA_PORT__ */
