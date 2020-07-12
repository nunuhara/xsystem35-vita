/*
 * Copyright (C) 2019 Nunuhara <nunuhara@kudaranai.ninja>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
#include <SDL.h>

extern SceUInt64 _process_time;

enum {
	VITA_TRIANGLE       = 0,
	VITA_CIRCLE         = 1,
	VITA_CROSS          = 2,
	VITA_SQUARE         = 3,
	VITA_LEFT_SHOULDER  = 4,
	VITA_RIGHT_SHOULDER = 5,
	VITA_DOWN           = 6,
	VITA_LEFT           = 7,
	VITA_UP             = 8,
	VITA_RIGHT          = 9,
	VITA_SELECT         = 10,
	VITA_START          = 11,
	VITA_NR_BUTTONS     = 12
};

void va_alarm_handler();
int vita_joystick_map(int in_key, const char *out);
void vita_joystick_event(SDL_Event *e);

#define TIME_START() _process_time = sceKernelGetProcessTimeWide()
#define TIME_END(fun)												\
	do {															\
		SceUInt64 diff = sceKernelGetProcessTimeWide() - _process_time;	\
		NOTICE("%s ran in %llu\n", fun, diff);			\
	} while (0);

int vita_launcher(int *argc, char ***argv);

#endif /* __VITA_PORT__ */
