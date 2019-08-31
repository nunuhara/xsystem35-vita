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

#include <psp2/appmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/types.h>
#include <psp2/ctrl.h>
#include <vita2d.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "config.h"
#include "system.h"

#define MSGBUF_SIZE 4096
#define GAMES_MAX   256

#define W 960
#define H 544
#define CX (W/2)
#define CY (H/2)

#define COLOR_BG     RGBA8(0x28, 0x28, 0x28, 0xFF)
#define COLOR_FG     RGBA8(0xED, 0xDB, 0xB2, 0xFF)
#define COLOR_ERR    RGBA8(0xCC, 0x2F, 0x1D, 0xFF)
#define COLOR_SEL_BG RGBA8(0x50, 0x49, 0x45, 0xFF)
#define COLOR_SEL_FG RGBA8(0xD5, 0xC4, 0xA1, 0xFF)

static int nr_games = 0;
static struct game {
	char *path;
	char *title;
} _games[GAMES_MAX];

// Sorted list of games
static struct game *games[GAMES_MAX];

static vita2d_pgf *pgf;
static SceCtrlData ctrl, prev_ctrl;
static int menu_sel = 0;

static bool is_game_directory(const char *path)
{
	DIR *dir;
	struct dirent *ent;
	size_t len;
	bool found_ald = false;

	if (!(dir = opendir(path)))
		return false;

	while ((ent = readdir(dir))) {
		if (!ent->d_name)
			continue;
		len = strlen(ent->d_name);
		if (len >= 6 && !strcasecmp(ent->d_name + len - 6, "sa.ald")) {
			found_ald = true;
			break;
		}
	}

	closedir(dir);
	return found_ald;
}

static int check_game_directory(const char *path, const char *base, struct game *game)
{
	struct stat s;
	static char path_buf[PATH_MAX];

	if (stat(path, &s) < 0)
		return 0;
	if (!S_ISDIR(s.st_mode))
		return 0;
	if (!is_game_directory(path))
		return 0;

	// TODO: read game manifest for title, other info(?)
	strncpy(path_buf, path, PATH_MAX);
	game->path = strdup(path);
	game->title = strdup(base);

	return 1;
}

static DIR *open_home_directory(const char *path)
{
	int rc;
	DIR *dir;
	struct stat s;

	rc = stat(path, &s);
	if (rc < 0 && errno != ENOENT) {
		return NULL;
	} else if (rc < 0) {
		// create directory
		if (sceIoMkdir(path, SCE_S_IRWXU | SCE_S_IRWXG | SCE_S_IRWXO) < 0)
			return NULL;
	} else if (!S_ISDIR(s.st_mode)) {
		errno = ENOTDIR;
		return NULL;
	}

	if (!(dir = opendir(path)))
		return NULL;

	return dir;
}

static bool scan_games(void)
{
	DIR *dir;
	struct dirent *ent;
	static char path_buf[PATH_MAX];

	if (!(dir = open_home_directory(VITA_HOME)))
		return false;

	while ((ent = readdir(dir))) {
		if (!ent->d_name)
			continue;
		snprintf(path_buf, sizeof(path_buf), VITA_HOME "/%s", ent->d_name);
		nr_games += check_game_directory(path_buf, ent->d_name, &_games[nr_games]);
	}

	closedir(dir);
	return true;
}

static void sort_games(void)
{
	for (int i = 0; i < nr_games; i++)
		games[i] = &_games[i];

	for (int i = 0; i < nr_games; i++) {
		int least = i;
		for (int j = i + 1; j < nr_games; j++) {
			if (strcasecmp(games[j]->title, games[least]->title) < 0)
				least = j;
		}
		// swap
		if (least != i) {
			struct game *tmp = games[i];
			games[i] = games[least];
			games[least] = tmp;
		}
	}
}

// draw_text flags
enum {
	CENTER_HORI  = 1,
	CENTER_VERT  = 2,
	ALIGN_RIGHT  = 4,
	ALIGN_BOTTOM = 8
};

static void draw_text(int flags, int x, int y, unsigned int color,
		float size, const char *fmt, ...)
{
	va_list ap;
	static char buf[MSGBUF_SIZE];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (flags & CENTER_HORI)
		x -= vita2d_pgf_text_width(pgf, size, buf) / 2;
	else if (flags & ALIGN_RIGHT)
		x -= vita2d_pgf_text_width(pgf, size, buf);

	if (flags & CENTER_VERT)
		y -= vita2d_pgf_text_height(pgf, size, buf) / 2;
	else if (flags & ALIGN_BOTTOM)
		y -= vita2d_pgf_text_height(pgf, size, buf);

	vita2d_pgf_draw_text(pgf, x, y, color, size, buf);
}

static void draw_box(int x, int y, int w, int h, int thk, unsigned int color)
{
	vita2d_draw_rectangle(x,       y,       w,   thk, color); // top line
	vita2d_draw_rectangle(x,       y+h-thk, w,   thk, color); // bottom line
	vita2d_draw_rectangle(x,       y,       thk, h,   color); // left line
	vita2d_draw_rectangle(x+w-thk, y,       thk, h,   color); // right line
}

static bool b_pressed(int b)
{
	return (ctrl.buttons & b) && !(prev_ctrl.buttons & b);
}

static bool b_held(int b)
{
	return ctrl.buttons & b;
}

static void b_wait(int b)
{
	while (true) {
		sceCtrlPeekBufferPositive(0, &ctrl, 1);
		if (b_held(b))
			break;
	}
}

static void check_ctrl(void)
{
	sceCtrlPeekBufferPositive(0, &ctrl, 1);
}

static bool update(void)
{
	// update menu cursor
	if (b_pressed(SCE_CTRL_DOWN))
		menu_sel++;
	if (b_pressed(SCE_CTRL_UP))
		menu_sel--;
	if (menu_sel < 0)
		menu_sel = nr_games - 1;
	if (menu_sel >= nr_games)
		menu_sel = 0;

	if (b_pressed(SCE_CTRL_CIRCLE)) {
		chdir(games[menu_sel]->path);
		return true;
	}
	return false;
}

#define OPAD     64                 // outside padding
#define LINE_THK 3                  // border thickness
#define IPAD_PX  1                  // inner padding pixels
#define IPAD     (LINE_THK+IPAD_PX) // inner padding
#define IBOX     (OPAD+IPAD)        // location of table contents
#define TPAD     6                  // left padding of text
#define TPOS     (IBOX+TPAD)        // x location of text

static void draw(void)
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	// table border
	draw_box(OPAD, OPAD, W-OPAD*2, H-OPAD*2, LINE_THK, COLOR_FG);

	// game table
	for (int i = 0; i < nr_games; i++) {
		bool selected = menu_sel == i;
		unsigned int fg = selected ? COLOR_SEL_FG : COLOR_FG;
		if (selected)
			vita2d_draw_rectangle(IBOX, IBOX + i*24, W-IBOX*2, 24, COLOR_SEL_BG);
		draw_text(CENTER_VERT, TPOS, IBOX + i*24 + 26, fg, 1, games[i]->title);
	}

	vita2d_end_drawing();
	vita2d_swap_buffers();
}

static void die(const char *fmt, ...)
{
	static char buf[MSGBUF_SIZE];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	vita2d_start_drawing();
	vita2d_clear_screen();
	draw_text(CENTER_HORI, CX, CY-64, COLOR_ERR, 2, "ERROR");
	draw_text(CENTER_HORI, CX, CY,    COLOR_FG,  1, buf);
	vita2d_end_drawing();
	vita2d_swap_buffers();
	vita2d_wait_rendering_done();

	b_wait(SCE_CTRL_START);
	vita2d_fini();
	vita2d_free_pgf(pgf);
	sceKernelExitProcess(0);
}

int vita_launcher(int *arg, char ***argv)
{
	bool launch = false;

	vita2d_init();
	vita2d_set_clear_color(COLOR_BG);
	pgf = vita2d_load_default_pgf();

	if (!scan_games())
		die("Failed to scan game directory: '" VITA_HOME "'\n%s", strerror(errno));
	if (!nr_games)
		die("No games found");
	sort_games();

	check_ctrl();
	while (!launch) {
		prev_ctrl = ctrl;
		check_ctrl();
		launch = update();
		draw();
	}

	vita2d_fini();
	vita2d_free_pgf(pgf);
	return 0;
}
