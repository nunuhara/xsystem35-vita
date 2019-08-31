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

#include "config.h"

#include <stdlib.h>
#include <SDL.h>

#include "portab.h"
#include "system.h"
#include "key.h"
#include "sdl_private.h"

// Name -> key mapping.
static struct {
	const char *name;
	int key;
} joy_outputs[] = {
	{ "up",           KEY_UP },
	{ "down",         KEY_DOWN },
	{ "left",         KEY_LEFT },
	{ "right",        KEY_RIGHT },
	{ "enter",        KEY_ENTER },
	{ "escape",       KEY_ESCAPE },
	{ "mouse_left",   KEY_MOUSE_LEFT },
	{ "mouse_right",  KEY_MOUSE_RIGHT },
	{ "mouse_middle", KEY_MOUSE_MIDDLE }
};
#define NR_JOY_OUTPUTS (sizeof(joy_outputs)/sizeof(*joy_outputs))

// The active button mapping.
static int joy_map[] = {
	[VITA_TRIANGLE]       = KEY_ESCAPE,
	[VITA_CIRCLE]         = KEY_MOUSE_RIGHT,
	[VITA_CROSS]          = KEY_MOUSE_LEFT,
	[VITA_SQUARE]         = KEY_ENTER,
	[VITA_LEFT_SHOULDER]  = -1,
	[VITA_RIGHT_SHOULDER] = -1,
	[VITA_DOWN]           = KEY_DOWN,
	[VITA_LEFT]           = KEY_LEFT,
	[VITA_UP]             = KEY_UP,
	[VITA_RIGHT]          = KEY_RIGHT,
	[VITA_SELECT]         = -1,
	[VITA_START]          = KEY_MOUSE_MIDDLE
};

// Mapping from xsystem35 keys to SDL keys.
static int sdl_keys[] = {
	[KEY_MOUSE_LEFT]   = SDL_BUTTON_LEFT,
	[KEY_MOUSE_RIGHT]  = SDL_BUTTON_RIGHT,
	[KEY_MOUSE_MIDDLE] = SDL_BUTTON_MIDDLE,
	[KEY_UP]           = SDL_SCANCODE_UP,
	[KEY_DOWN]         = SDL_SCANCODE_DOWN,
	[KEY_LEFT]         = SDL_SCANCODE_LEFT,
	[KEY_RIGHT]        = SDL_SCANCODE_RIGHT,
	[KEY_ENTER]        = SDL_SCANCODE_RETURN,
	[KEY_ESCAPE]       = SDL_SCANCODE_ESCAPE
};

float joydir_x = 0;
float joydir_y = 0;

int vita_joystick_map(int in_key, const char *out)
{
	int out_key = -1;
	for (int i = 0; i < NR_JOY_OUTPUTS; i++) {
		if (!strcasecmp(out, joy_outputs[i].name)) {
			out_key = joy_outputs[i].key;
			break;
		}
	}

	if (in_key < 0 || in_key >= VITA_NR_BUTTONS || out_key < 0)
		return 0;

	joy_map[in_key] = out_key;
	return 1;
}

int vita_joystick_get(int button)
{
	if (button < 0 || button >= VITA_NR_BUTTONS)
		return -1;
	return joy_map[button];
}

// XXX: The SDL docs claim that the memory for an SDL_Event can be disposed of
//      after calling SDL_PushEvent. Yet putting this variable on the stack
//      causes e.type to become zero in sdl_getEvent.
static SDL_Event e;

static void push_mouse_event(int button, int pressed)
{
	e.type = pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	e.button.button = button;
	SDL_PushEvent(&e);
}

static void push_key_event(int key, int pressed)
{
	e.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
	e.key.keysym.scancode = key;
	SDL_PushEvent(&e);
}

static int key_to_sdl(int key)
{
	return sdl_keys[key];
}

void vita_joystick_event(SDL_Event *e)
{
	int b;

	switch (e->type) {
	case SDL_JOYAXISMOTION:
		if (e->jaxis.axis == 0) {
			joydir_x = e->jaxis.value/32768.0;
		} else if (e->jaxis.axis == 1) {
			joydir_y = e->jaxis.value/32768.0;
		}
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		if ((b = vita_joystick_get(e->jbutton.button)) < 0)
			break;
		if (b == KEY_MOUSE_LEFT || b == KEY_MOUSE_RIGHT || b == KEY_MOUSE_MIDDLE) {
			push_mouse_event(key_to_sdl(b), e->type == SDL_JOYBUTTONDOWN);
		} else {
			push_key_event(key_to_sdl(b), e->type == SDL_JOYBUTTONDOWN);
		}
		break;
	default:
		break;
	}
}
