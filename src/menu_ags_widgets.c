#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "portab.h"
#include "system.h"
#include "ags.h"
#include "imput.h"
#include "key.h"
#include "menu_ags.h"

#define DEFAULT_FONT_SIZE 16
#define WIDGET_STACK_MAX 16

struct widget *widget_stack[WIDGET_STACK_MAX];
int widget_stack_ptr = -1;

void widget_activate(struct widget *w)
{
	if (w->activate)
		w->activate(w);
}

void widget_draw(struct widget *w)
{
	if (w->draw)
		w->draw(w);
}

void widget_handle_key(struct widget *w, BYTE key, boolean pressed)
{
	if (w->handle_key)
		w->handle_key(w, key, pressed);
}

void widget_handle_mouse(struct widget *w, MyPoint cur, MyPoint prev)
{
	if (w->handle_mouse)
		w->handle_mouse(w, cur, prev);
}

void widget_pack(struct widget *w)
{
	if (w->pack)
		w->pack(w);
}

void widget_free(struct widget *w)
{
	if (w->free)
		w->free(w);
	else
		free(w);
}

void widget_stack_push(struct widget *w)
{
	if (widget_stack_ptr + 1 >= WIDGET_STACK_MAX)
		SYSERROR("Widget stack overflowed");
	widget_stack[++widget_stack_ptr] = w;
	menu_dirty = TRUE;
}

void widget_stack_pop(void)
{
	struct widget *top = widget_stack[widget_stack_ptr--];
	widget_free(top);
	menu_dirty = TRUE;
}

struct widget *widget_stack_peek(void)
{
	if (widget_stack_ptr < 0)
		return NULL;
	return widget_stack[widget_stack_ptr];
}

void widget_stack_flush(void)
{
	while (widget_stack_ptr >= 0) {
		widget_stack_pop();
	}
}

/*
 * Menu widget.
 */

static void menu_pack(struct widget *w)
{
	DispInfo d;
	int max_w = 0;
	int height = 0;
	struct menu *menu = (struct menu*)w;

	// pack children
	for (int i = 0; i < menu->nr_entries; i++) {
		widget_pack(menu->entries[i]);
		max_w = max(max_w, menu->entries[i]->geo.width);
		height += menu->entries[i]->geo.height;
	}

	// set menu dimensions
	ags_getViewAreaInfo(&d);
	w->geo.width = max_w + 4;
	w->geo.height = height + 4;
	w->geo.x = d.width/2 - w->geo.width/2;
	w->geo.y = d.height/2 - w->geo.height/2;

	// set children (x,y) coordinates
	for (int i = 0, y = w->geo.y + 2; i < menu->nr_entries; i++) {
		menu->entries[i]->geo.x = w->geo.x + 2;
		menu->entries[i]->geo.y = y;
		menu->entries[i]->geo.width = max_w;
		y += menu->entries[i]->geo.height;
	}
}

static void draw_frameborder(MyRectangle r, int frame_color, int bg_color)
{
	ags_drawRectangle(r.x - 8, r.y - 8, r.width + 16, r.height + 16, frame_color);
	ags_drawRectangle(r.x - 7, r.y - 7, r.width + 14, r.height + 14, frame_color);
	ags_drawRectangle(r.x - 6, r.y - 6, r.width + 12, r.height + 12, frame_color);
	ags_drawRectangle(r.x - 5, r.y - 5, r.width + 10, r.height + 10, bg_color);
	ags_drawRectangle(r.x - 4, r.y - 4, r.width +  8, r.height +  8, bg_color);
	ags_drawRectangle(r.x - 3, r.y - 3, r.width +  6, r.height +  6, frame_color);
	ags_drawRectangle(r.x - 2, r.y - 2, r.width +  4, r.height +  4, bg_color);
	ags_drawRectangle(r.x - 1, r.y - 1, r.width +  2, r.height +  2, bg_color);
	ags_fillRectangle(r.x,     r.y,     r.width,      r.height,      bg_color);
}

static void menu_draw(struct widget *w)
{
	struct menu *m = (struct menu*)w;

	draw_frameborder(w->geo, m->frame_color, m->bg_color);

	// draw children
	for (int i = 0; i < m->nr_entries; i++) {
		widget_draw(m->entries[i]);
		if (i == m->selection) {
			MyRectangle *r = &m->entries[i]->geo;
			ags_drawRectangle(r->x-2, r->y-2, r->width+4, r->height+4, m->fg_color);
			ags_drawRectangle(r->x-1, r->y-1, r->width+2, r->height+2, m->fg_color);
		}
	}
}

static int menu_entry_at(struct menu *m, int x, int y)
{
	if (ags_regionContains(&m->w.geo, x, y)) {
		for (int i = 0; i < m->nr_entries; i++) {
			if (ags_regionContains(&m->entries[i]->geo, x, y)) {
				return i;
			}
		}
	}
	return -1;
}

static void menu_handle_key(struct widget *w, BYTE key, boolean pressed)
{
	MyPoint mouse;
	int hovered;
	struct menu *menu = (struct menu*)w;

	if (pressed) {
		switch (key) {
		case KEY_UP:
			menu->selection = max(0, menu->selection - 1);
			return;
		case KEY_DOWN:
			menu->selection = min(menu->nr_entries - 1, menu->selection + 1);
			return;
		default:
			break;
		}
	} else {
		switch (key) {
		case KEY_MOUSE_LEFT:
			sys_getMouseInfo(&mouse, FALSE);
			hovered = menu_entry_at(menu, mouse.x, mouse.y);
			if (hovered >= 0) {
				widget_activate(menu->entries[hovered]);
			}
			return;
		case KEY_ENTER:
			widget_activate(menu->entries[menu->selection]);
			return;
		case KEY_ESC:
			widget_stack_pop();
			return;
		default:
			break;
		}
	}
	widget_handle_key(menu->entries[menu->selection], key, pressed);
}

static void menu_handle_mouse(struct widget *w, MyPoint cur, MyPoint prev)
{
	struct menu *menu = (struct menu*)w;
	int hovered = menu_entry_at(menu, cur.x, cur.y);

	if (hovered >= 0) {
		if (hovered != menu->selection)
			menu_dirty = TRUE;
		menu->selection = hovered;
	}
}

static void menu_free(struct widget *w)
{
	struct menu *m = (struct menu*)w;

	for (int i = 0; i < m->nr_entries; i++) {
		widget_free(m->entries[i]);
	}

	free(m->entries);
	free(m);
}

struct menu *make_menu(void)
{
	struct menu *m = calloc(1, sizeof(struct menu));
	m->w.pack = menu_pack;
	m->w.draw = menu_draw;
	m->w.handle_key = menu_handle_key;
	m->w.handle_mouse = menu_handle_mouse;
	m->w.free = menu_free;

	m->bg_color = default_bg_color;
	m->fg_color = default_fg_color;
	m->frame_color = default_frame_color;
	m->font_size = default_font_size;

	return m;
}

void menu_append_entry(struct menu *m, struct widget *w)
{
	m->nr_entries++;
	m->entries = realloc(m->entries, m->nr_entries * sizeof(struct widget*));
	m->entries[m->nr_entries-1] = w;
}

/*
 * Label widget.
 */

static void label_pack(struct widget *w)
{
	struct label *label = (struct label*)w;
	w->geo.width = strlen(label->text) * (label->font_size/2);
	w->geo.height = label->font_size;
}

static void label_draw(struct widget *w)
{
	struct label *label = (struct label*)w;
	ags_setFont(FONT_GOTHIC, label->font_size);
	ags_drawString(w->geo.x, w->geo.y, label->text, label->color);
}

static void label_free(struct widget *w)
{
	struct label *label = (struct label*)w;
	free(label->text);
	free(label);
}

struct label *make_label(const char *msg, void (*activate)(struct widget*))
{
	struct label *label = calloc(1, sizeof(struct label));
	label->w.pack = label_pack;
	label->w.draw = label_draw;
	label->w.free = label_free;
	label->w.activate = activate;
	label->text = strdup(msg);
	label->font_size = default_font_size;
	label->color = default_fg_color;
}

#define CHAR_OFF "\x81\x9b" // empty circle
#define CHAR_ON  "\x81\x9c" // solid circle

/*
 * Toggle widget.
 */

static void toggle_pack(struct widget *w)
{
	struct toggle *toggle = (struct toggle*)w;
	w->geo.width = (strlen(toggle->text) + sizeof(CHAR_ON)) * (toggle->font_size/2);
	w->geo.height = toggle->font_size;
}

static void toggle_draw(struct widget *w)
{
	struct toggle *toggle = (struct toggle*)w;
	int text_x = w->geo.x + (toggle->font_size * sizeof(CHAR_ON))/2;
	ags_setFont(FONT_GOTHIC, toggle->font_size);
	ags_drawString(w->geo.x, w->geo.y, toggle->on ? CHAR_ON : CHAR_OFF, toggle->color);
	ags_drawString(text_x, w->geo.y, toggle->text, toggle->color);
}

static void toggle_free(struct widget *w)
{
	struct toggle *toggle = (struct toggle*)w;
	free(toggle->text);
	free(toggle);
}

static void toggle_activate(struct widget *w)
{
	struct toggle *toggle = (struct toggle*)w;
	toggle->on = toggle->toggle(!toggle->on);
	menu_dirty = TRUE;
}

struct toggle *make_toggle(const char *msg, boolean(*toggle_cb)(boolean))
{
	struct toggle *toggle = calloc(1, sizeof(struct toggle));
	toggle->w.pack = toggle_pack;
	toggle->w.draw = toggle_draw;
	toggle->w.free = toggle_free;
	toggle->w.activate = toggle_activate;
	toggle->text = strdup(msg);
	toggle->font_size = default_font_size;
	toggle->color = default_fg_color;
	toggle->toggle = toggle_cb;
}
