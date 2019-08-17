/*
 * sdl_darw.c  SDL event handler
 *
 * Copyright (C) 2000-     Fumihiko Murata       <fmurata@p1.tcnet.ne.jp>
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
 *
*/
/* $Id: sdl_event.c,v 1.5 2001/12/16 17:12:56 chikama Exp $ */

#include "config.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL.h>

#include "portab.h"
#include "system.h"
#include "counter.h"
#include "nact.h"
#include "sdl_private.h"
#include "key.h"
#include "menu.h"
#include "imput.h"
#include "joystick.h"
#include "sdl_input.c"

static void sdl_getEvent(void);
static void keyEventProsess(SDL_KeyboardEvent *e, boolean bool);
static int  check_button(void);

/* pointer の状態 */
int mousex, mousey, mouseb;
boolean RawKeyInfo[256];
/* SDL Joystick */
static int joyinfo=0;

#ifdef VITA
#define JOYSTICK_DEAD_ZONE 8000
#define JOYSTICK_SPEED 2
extern void va_alarm_handler();
static int joydir_x = 0;
static int joydir_y = 0;
SceUInt64 joy_time = 0;
boolean hide_cursor = TRUE;
#endif

/*
 * Translate touch screen coordinates to logical coordinates.
 * Returns true IFF the touch location is within the display bounds.
 */
static boolean touch_location(SDL_Event *e, int *x, int *y) {
#ifdef VITA
	int _x = (e->tfinger.x*VITA_W - renderoffset_x) * (1/renderscale);
	int _y = (e->tfinger.y*VITA_H - renderoffset_y) * (1/renderscale);
	NOTICE("TOUCH %d %d\n", _x, _y);
	if (_x < 0 || _y < 0 || _x >= view_w || _y >= view_h)
		return FALSE;
	*x = _x;
	*y = _y;
#else
	*x = e->tfinger.x * view_w;
	*y = e->tfinger.y * view_h;
#endif
	return TRUE;
}

static int mouse_to_rawkey(int button) {
	switch(button) {
	case SDL_BUTTON_LEFT:
		return KEY_MOUSE_LEFT;
	case SDL_BUTTON_MIDDLE:
		return KEY_MOUSE_MIDDLE;
	case SDL_BUTTON_RIGHT:
		return KEY_MOUSE_RIGHT;
	}
	return 0;
}

static int mouse_to_agsevent(int button) {
	switch(button) {
	case SDL_BUTTON_LEFT:
		return AGSEVENT_BUTTON_LEFT;
	case SDL_BUTTON_MIDDLE:
		return AGSEVENT_BUTTON_MID;
	case SDL_BUTTON_RIGHT:
		return AGSEVENT_BUTTON_RIGHT;
	}
	return 0;
}

static void send_agsevent(int type, int code) {
	if (!nact->ags.eventcb)
		return;
	agsevent_t agse;
	agse.type = type;
	agse.d1 = mousex;
	agse.d2 = mousey;
	agse.d3 = code;
	nact->ags.eventcb(&agse);
}

static void mouse_down(Uint8 button)
{
	mouseb |= (1 << button);
	RawKeyInfo[mouse_to_rawkey(button)] = TRUE;
	send_agsevent(AGSEVENT_BUTTON_PRESS, mouse_to_agsevent(button));
#if 0
	if (button == 2) {
		keywait_flag=TRUE;
	}
#endif
}

static boolean mouse_up(Uint8 button, boolean *m2b)
{
	mouseb &= (0xffffffff ^ (1 << button));
	RawKeyInfo[mouse_to_rawkey(button)] = FALSE;
	send_agsevent(AGSEVENT_BUTTON_RELEASE, mouse_to_agsevent(button));
	if (button == 2) {
		*m2b = TRUE;
	}
}

/* Event処理 */
static void sdl_getEvent(void) {
	static int cmd_count_of_prev_input = -1;
	SDL_Event e;
	boolean m2b = FALSE, msg_skip = FALSE;
	int i;
	boolean had_input = false;

	while (SDL_PollEvent(&e)) {
		had_input = true;
		switch (e.type) {
#ifndef __EMSCRIPTEN__
		case SDL_QUIT:
			nact->is_quit = TRUE;
			nact->wait_vsync = TRUE;
			break;
#endif
		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_ENTER:
				ms_active = true;
#if 0
				if (sdl_fs_on)
					SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
				break;
			case SDL_WINDOWEVENT_LEAVE:
				ms_active = false;
#if 0
				if (sdl_fs_on)
					SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				sdl_dirty = TRUE;
				break;
			}
			break;
		case SDL_KEYDOWN:
			keyEventProsess(&e.key, TRUE);
			break;
		case SDL_KEYUP:
			keyEventProsess(&e.key, FALSE);
			if (e.key.keysym.sym == SDLK_F1) msg_skip = TRUE;
#ifndef __EMSCRIPTEN__
			if (e.key.keysym.sym == SDLK_F4) {
				sdl_fs_on = !sdl_fs_on;
				SDL_SetWindowFullscreen(sdl_window, sdl_fs_on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
			}
#endif
			break;
		case SDL_MOUSEMOTION:
			mousex = e.motion.x;
			mousey = e.motion.y;
			send_agsevent(AGSEVENT_MOUSE_MOTION, 0);
			break;
		case SDL_MOUSEBUTTONDOWN:
			mouse_down(e.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			mouse_up(e.button.button, &m2b);
			break;

		case SDL_FINGERDOWN:
			if (e.tfinger.touchId != 0)
				break;
			if (SDL_GetNumTouchFingers(SDL_GetTouchDevice(0)) >= 2) {
				mouse_up(SDL_BUTTON_LEFT, &m2b);
				mouse_down(SDL_BUTTON_RIGHT);
			} else {
				if (touch_location(&e, &mousex, &mousey))
					mouse_down(SDL_BUTTON_LEFT);
			}
			break;

		case SDL_FINGERUP:
			if (e.tfinger.touchId != 0)
				break;
			if (SDL_GetNumTouchFingers(SDL_GetTouchDevice(0)) == 0) {
				if (touch_location(&e, &mousex, &mousey)) {
					mouse_up(SDL_BUTTON_LEFT, &m2b);
					mouse_up(SDL_BUTTON_RIGHT, &m2b);
				}
			}
			break;

		case SDL_FINGERMOTION:
			if (e.tfinger.touchId != 0)
				break;
			if (touch_location(&e, &mousex, &mousey))
				send_agsevent(AGSEVENT_MOUSE_MOTION, 0);
			break;

#ifdef VITA
		case SDL_JOYAXISMOTION:
			if (e.jaxis.axis == 0) {
				// Left of dead zone
				if (e.jaxis.value < -JOYSTICK_DEAD_ZONE)
					joydir_x = -1;
				else if (e.jaxis.value > JOYSTICK_DEAD_ZONE)
					joydir_x = 1;
				else
					joydir_x = 0;
			} else if (e.jaxis.axis == 1) {
				if (e.jaxis.value < -JOYSTICK_DEAD_ZONE)
					joydir_y = -1;
				else if (e.jaxis.value > JOYSTICK_DEAD_ZONE)
					joydir_y = 1;
				else
					joydir_y = 0;
			}
			break;
		case SDL_JOYBUTTONDOWN:
			NOTICE("BUTTON: %d\n", e.jbutton.button);
			switch (e.jbutton.button) {
			case 1: mouse_down(SDL_BUTTON_RIGHT); break; // Circle
			case 2: mouse_down(SDL_BUTTON_LEFT);  break; // X
			// TODO: if possible, use D-pad for menus instead of mouse
			case 6: joydir_y =  1;                break; // Down
			case 7: joydir_x = -1;                break; // Left
			case 8: joydir_y = -1;                break; // Up
			case 9: joydir_x =  1;                break; // Right
			default:                              break; // Ignore others
			}
			break;
		case SDL_JOYBUTTONUP:
			switch (e.jbutton.button) {
			case 1: mouse_up(SDL_BUTTON_RIGHT, &m2b); break; // Circle
			case 2: mouse_up(SDL_BUTTON_LEFT, &m2b);  break; // X
			case 6: case 8: joydir_y = 0;             break; // Down/Up
			case 7: case 9: joydir_x = 0;             break; // Down/Up
			default:                                  break; // Ignore others
			}
			break;
		case SDL_USEREVENT:
			va_alarm_handler();
			break;
#else
#if HAVE_SDLJOY
		case SDL_JOYAXISMOTION:
			if (abs(e.jaxis.value) < 0x4000) {
				joyinfo &= e.jaxis.axis == 0 ? ~0xc : ~3;
			} else {
				i = (e.jaxis.axis == 0 ? 2 : 0) + 
					(e.jaxis.value > 0 ? 1 : 0);
				joyinfo |= 1 << i;
			}
			break;
		case SDL_JOYBALLMOTION:
			break;
		case SDL_JOYHATMOTION:
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			if (e.jbutton.button < 4) {
				i = 1 << (e.jbutton.button+4);
				if (e.jbutton.state == SDL_PRESSED)
					joyinfo |= i;
				else
					joyinfo &= ~i;
			} else {
				if (e.jbutton.state == SDL_RELEASED) {
				}
			}
			break;
#endif // HAVE_SDLJOY
#endif // VITA
		default:
			printf("ev %x\n", e.type);
			break;
		}
	}
#ifdef VITA
	if (joydir_x || joydir_y) {
		joy_time = sceKernelGetProcessTimeWide();
		// TODO: variable speed
		mousex = max(0, min(view_w-1, mousex + joydir_x * JOYSTICK_SPEED));
		mousey = max(0, min(view_h-1, mousey + joydir_y * JOYSTICK_SPEED));
		send_agsevent(AGSEVENT_MOUSE_MOTION, 0);
		hide_cursor = FALSE;
		sdl_dirty = TRUE;
	} else if (!hide_cursor && sceKernelGetProcessTimeWide() - joy_time > 750000) {
		hide_cursor = TRUE;
		sdl_dirty = TRUE;
	}
#endif
	if (had_input) {
		cmd_count_of_prev_input = nact->cmd_count;
	} else if (nact->cmd_count != cmd_count_of_prev_input) {
		nact->wait_vsync = TRUE;
	}
	
	if (m2b) {
		menu_open();
	}
	
	if (msg_skip) set_skipMode(!get_skipMode());
}

int sdl_keywait(int msec, boolean cancel) {
	int key=0, n;
	int end = msec == INT_MAX ? INT_MAX : get_high_counter(SYSTEMCOUNTER_MSEC) + msec;
	
	while ((n = end - get_high_counter(SYSTEMCOUNTER_MSEC)) > 0) {
		if (n <= 16)
			sdl_sleep(n);
		else
			sdl_wait_vsync();
		nact->callback();
		sdl_getEvent();
		key = check_button() | sdl_getKeyInfo() | joy_getinfo();
		nact->wait_vsync = FALSE;  // We just waited!
		if (cancel && key) break;
	}
	
	return key;
}

/* キー情報の取得 */
static void keyEventProsess(SDL_KeyboardEvent *e, boolean bool) {
	int code = sdl_keytable[e->keysym.scancode];
	RawKeyInfo[code] = bool;
	send_agsevent(bool ? AGSEVENT_KEY_PRESS : AGSEVENT_KEY_RELEASE, code);
}

int sdl_getKeyInfo() {
	int rt;
	
	sdl_getEvent();
	
	rt = ((RawKeyInfo[KEY_UP]     || RawKeyInfo[KEY_PAD_8])       |
	      ((RawKeyInfo[KEY_DOWN]  || RawKeyInfo[KEY_PAD_2]) << 1) |
	      ((RawKeyInfo[KEY_LEFT]  || RawKeyInfo[KEY_PAD_4]) << 2) |
	      ((RawKeyInfo[KEY_RIGHT] || RawKeyInfo[KEY_PAD_6]) << 3) |
	      (RawKeyInfo[KEY_RETURN] << 4) |
	      (RawKeyInfo[KEY_SPACE ] << 5) |
	      (RawKeyInfo[KEY_ESC]    << 6) |
	      (RawKeyInfo[KEY_TAB]    << 7));
	
	return rt;
}

static int check_button(void) {
	int m1, m2;
	
	m1 = mouseb & (1 << 1) ? SYS35KEY_RET : 0;
	m2 = mouseb & (1 << 3) ? SYS35KEY_SPC : 0;
	
	return m1 | m2;
}

int sdl_getMouseInfo(MyPoint *p) {
	sdl_getEvent();
	
	if (!ms_active) {
		if (p) {
			p->x = 0; p->y = 0;
		}
		return 0;
	}
	
	if (p) {
		p->x = mousex - winoffset_x;
		p->y = mousey - winoffset_y;
	}
	return check_button();
}

#ifdef HAVE_SDLJOY
int sdl_getjoyinfo(void) {
	sdl_getEvent();
	return joyinfo;
}
#endif
