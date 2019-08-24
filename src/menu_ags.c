#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "config.h"
#include "portab.h"
#include "ags.h"
#include "graphicsdevice.h"
#include "imput.h"
#include "key.h"
#include "menu.h"
#include "menu_ags.h"
#include "nact.h"

#define CHAR_RARROW "\x81\xa8"

int default_bg_color = 255;
int default_fg_color = 0;
int default_frame_color = 0;
int default_font_size = 16;

static struct {
	void *region;
	int font_type;
	int font_size;
	void (*eventcb)(agsevent_t*);
} saved;

static boolean toggle_fullscreen(boolean on)
{
	ags_fullscreen(on);
	return nact->sys_fullscreen_on;
}

static boolean toggle_skipmode(boolean on)
{
	set_skipMode(on);
	return get_skipMode();
}

static boolean toggle_mousemove(boolean on)
{
	nact->sys_mouse_movesw = on ? 2 : 0;
	return !!on;
}

static void activate_quit(struct widget *w)
{
	nact->is_quit = TRUE;
}

static void activate_return(struct widget *w)
{
	widget_stack_flush();
}

static struct menu *make_main_menu(void)
{
	struct toggle *toggle;
	struct menu *menu = make_menu();

	toggle = make_toggle("Fullscreen", toggle_fullscreen);
	toggle->on = nact->sys_fullscreen_on;
	menu_append_entry(menu, (struct widget*)toggle);

	toggle = make_toggle("Skip Text", toggle_skipmode);
	toggle->on = get_skipMode();
	menu_append_entry(menu, (struct widget*)toggle);

	toggle = make_toggle("Mouse Movement", toggle_mousemove);
	toggle->on = nact->sys_mouse_movesw;
	menu_append_entry(menu, (struct widget*)toggle);

	menu_append_entry(menu, (struct widget*)make_label (CHAR_RARROW " Quit", activate_quit));
	menu_append_entry(menu, (struct widget*)make_label (CHAR_RARROW " Return", activate_return));

	widget_pack((struct widget*)menu);
	return menu;
}

static int find_nearest_color(int r, int g, int b)
{
	int nearest = 0;
	int nearest_diff = INT_MAX;
	for (int i = 0; i < 256; i++) {
		int diff = 0;
		diff += abs(nact->sys_pal->red[i] - r);
		diff += abs(nact->sys_pal->green[i] - g);
		diff += abs(nact->sys_pal->blue[i] - b);
		if (diff < nearest_diff) {
			nearest = i;
			nearest_diff = diff;
		}
	}
	return nearest;
}

void menu_open(void) {
	// only open once
	if (nact->popupmenu_opened)
		return;

	nact->popupmenu_opened = TRUE;

	DispInfo d;
	ags_getViewAreaInfo(&d);
	saved.region = ags_saveRegion(0, 0, d.width, d.height);
	saved.font_type = nact->ags.font_type;
	saved.font_size = nact->ags.font_size;
	saved.eventcb   = nact->ags.eventcb;

	// Kichikuou menu colors
	default_bg_color    = find_nearest_color(190, 195, 220);
	default_fg_color    = find_nearest_color(110, 115, 130);
	default_frame_color = find_nearest_color(255, 255, 255);

	nact->ags.eventcb = NULL;

	widget_stack_push((struct widget*)make_main_menu());
}

void menu_quitmenu_open(void) {
	return;
}

boolean menu_inputstring(INPUTSTRING_PARAM *p) {
	p->newstring = p->oldstring;
	return TRUE;
}

boolean menu_inputstring2(INPUTSTRING_PARAM *p) {
	p->newstring = p->oldstring;
	return TRUE;
}

boolean menu_inputnumber(INPUTNUM_PARAM *p) {
	p->value = p->def;
	return TRUE;
}

void menu_msgbox_open(char *msg) {
	return;
}

void menu_init(void) {
	return;
}

void menu_widget_reinit(boolean reset_colortmap) {
	return;
}

static MyPoint mouse_state;
static boolean kbd_state[256];

static void menu_close(void)
{
	nact->popupmenu_opened = FALSE;
	ags_restoreRegion(saved.region, 0, 0);
	ags_setFont(saved.font_type, saved.font_size);
	nact->ags.eventcb = saved.eventcb;
	ags_updateFull();

	memset(&saved, 0, sizeof(saved));
	memset(&mouse_state, 0, sizeof(mouse_state));
	memset(&kbd_state, 0, sizeof(kbd_state));
}

static void handle_input(struct widget *current)
{
	MyPoint p;
	sys_getMouseInfo(&p, FALSE);

	// middle-mouse closes the menu
	if (!RawKeyInfo[KEY_MOUSE_MIDDLE] && kbd_state[KEY_MOUSE_MIDDLE]) {
		widget_stack_flush();
		return;
	}

	// handle mouse movement
	if (abs(p.x - mouse_state.x) > 0 || abs(p.y - mouse_state.y) > 0) {
		widget_handle_mouse(current, p, mouse_state);
	}

	// handle key events
	if (current->handle_key) {
		int nr_pressed = 0;
		int nr_released = 0;
		BYTE key_press[256];
		BYTE key_release[256];

		// check for press/release events
		for (int i = 1; i < 256; i++) {
			if (RawKeyInfo[i] == kbd_state[i])
				continue;
			if (RawKeyInfo[i]) {
				key_press[nr_pressed++] = i;
			} else {
				key_release[nr_released++] = i;
			}
		}

		// deliver events to active widget
		for (int i = 0; i < nr_pressed; i++) {
			widget_handle_key(current, key_press[i], TRUE);
		}
		for (int i = 0; i < nr_released; i++) {
			widget_handle_key(current, key_release[i], FALSE);
		}
	}

	mouse_state = p;
	memcpy(kbd_state, RawKeyInfo, sizeof(kbd_state));
}

void menu_gtkmainiteration() {
	// prevent recursion
	static boolean in_menu = FALSE;
	if (in_menu)
		return;
	in_menu = TRUE;

	struct widget *current;
	while ((current = widget_stack_peek())) {
		if (menu_dirty)
			widget_draw(current);
		handle_input(current);
		ags_updateFull();
		WaitVsync();
		nact->callback();
	}

	menu_close();
	in_menu = FALSE;
	return;
}
