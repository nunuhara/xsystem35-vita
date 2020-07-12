#ifndef __MENU_AGS__
#define __MENU_AGS__

#include "portab.h"
#include "graphics.h"

struct widget {
	// widget geometry
	MyRectangle geo;

	// Set x,y,w,h for the widget and it's children.
	// Must be called at least once before drawing.
	void (*pack)(struct widget*);

	// Draw the widget at its configured location/size.
	void (*draw)(struct widget*);

	// Called when a widget is "activated", e.g. pressing Enter in a menu.
	void (*activate)(struct widget*);

	// Handle keyboard input events.
	void (*handle_key)(struct widget*, BYTE, boolean);

	// Handle mouse motion events.
	void (*handle_mouse)(struct widget*, MyPoint, MyPoint);

	// Free the widget. NULL = stdlib free().
	void (*free)(struct widget*);
};

struct menu {
	struct widget w;
	int bg_color;
	int fg_color;
	int frame_color;
	int font_size;
	int selection;
	int hovered;
	int nr_entries;
	struct widget **entries;
};

struct label {
	struct widget w;
	char *text;
	int font_size;
	int color;
};

struct toggle {
	struct widget w;
	char *text;
	int font_size;
	int color;
	boolean on;
	boolean (*toggle)(boolean);
};

#define MSGBOX_MAX_LINES 128

struct msgbox {
	struct widget w;
	int nr_lines;
	char *lines[MSGBOX_MAX_LINES];
	int bg_color;
	int fg_color;
	int frame_color;
	int font_size;
	int selection;
	int nr_buttons;
	struct label **buttons;
};

extern boolean menu_dirty;
extern int default_bg_color;
extern int default_fg_color;
extern int default_frame_color;
extern int default_font_size;

void widget_activate(struct widget *w);
void widget_draw(struct widget *w);
void widget_handle_key(struct widget *w, BYTE key, boolean pressed);
void widget_handle_mouse(struct widget *w, MyPoint cur, MyPoint prev);
void widget_pack(struct widget *w);
void widget_free(struct widget *w);

void widget_stack_push(struct widget *w);
void widget_stack_pop(void);
struct widget *widget_stack_peek(void);
void widget_stack_flush(void);
void widget_stack_draw(void);

struct menu *make_menu(void);
void menu_append_entry(struct menu *m, struct widget *w);

struct msgbox *make_msgbox(const char *message);
void msgbox_add_button(struct msgbox *box, struct label *button);

struct label *make_label(const char *msg, void (*activate)(struct widget*));

struct toggle *make_toggle(const char *msg, boolean(*toggle_cb)(boolean));

#endif /* __MENU_AGS__ */
