/*
 * sdl_darw.c  SDL video mode and full-screen
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
/* $Id: sdl_mode.c,v 1.6 2003/01/04 17:01:02 chikama Exp $ */

#include "config.h"

#include <stdio.h>
#include <limits.h>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "portab.h"
#include "system.h"
#include "sdl_private.h"

#ifdef VITA
static void update_renderparams(void)
{
	if (sdl_fs_on) {
		float hscale = (float)VITA_W / (float)view_w;
		float vscale = (float)VITA_H / (float)view_h;
		renderscale = min(hscale, vscale);
	} else {
		renderscale = 1;
	}
	renderoffset_x = 960/2 - (view_w*renderscale)/2;
	renderoffset_y = 544/2 - (view_h*renderscale)/2;
}

#endif

void sdl_FullScreen(boolean on) {
#ifdef VITA
	sdl_fs_on = on;
	sdl_dirty = TRUE;
	update_renderparams();
#else
	if (on && !sdl_fs_on) {
		sdl_fs_on = TRUE;
		SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else if (!on && sdl_fs_on) {
		sdl_fs_on = FALSE;
		SDL_SetWindowFullscreen(sdl_window, 0);
	}
#endif
}

/* Windowの大きさの変更 */
void sdl_setWindowSize(int x, int y, int w, int h) {
 	view_x = x;
	view_y = y;
	
	if (w == view_w && h == view_h) return;
	
	view_w = w;
	view_h = h;
	
#ifndef __ANDROID__
	SDL_SetWindowSize(sdl_window, w, h);
#endif
	if (sdl_display)
		SDL_FreeSurface(sdl_display);
	if (sdl_texture)
		SDL_DestroyTexture(sdl_texture);
	sdl_display = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
	sdl_texture = SDL_CreateTexture(sdl_renderer, sdl_display->format->format,
									SDL_TEXTUREACCESS_STATIC, w, h);
#ifdef VITA
	update_renderparams();
#else
	SDL_RenderSetLogicalSize(sdl_renderer, w, h);
#endif

	//ms_active = (SDL_GetAppState() & SDL_APPMOUSEFOCUS) ? TRUE : FALSE;
#ifdef __EMSCRIPTEN__
	EM_ASM( xsystem35.shell.windowSizeChanged(); );
#endif
}
