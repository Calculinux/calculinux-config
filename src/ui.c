#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <ncurses.h>

#include "ui.h"

#define ROW_TITLE  0
#define ROW_STATUS 1
#define ROW_DIV    2
#define ROW_LIST   3

#define CP_TITLE    1
#define CP_SELECTED 2
#define CP_STATUS   3
#define CP_ERROR    4
#define CP_OK       5

static char   g_title[48]  = "Calculinux Config";
static char   g_status[80] = "";
static char   g_hints[80]  = "[j/k] Move  [Enter] Select  [q] Back";
static UiItem g_items[UI_MAX_ITEMS];
static int    g_count;
static int    g_cursor;
static int    g_offset;
static int    g_list_rows;
static int    g_hint_row;

static void init_colors(void)
{
	start_color();
	use_default_colors();
	init_pair(CP_TITLE,    COLOR_BLACK, COLOR_GREEN);
	init_pair(CP_SELECTED, COLOR_BLACK, COLOR_WHITE);
	init_pair(CP_STATUS,   -1,          -1);
	init_pair(CP_ERROR,    COLOR_WHITE, COLOR_RED);
	init_pair(CP_OK,       COLOR_GREEN, -1);
}

static void recompute_layout(void)
{
	g_hint_row  = LINES - 1;
	g_list_rows = g_hint_row - ROW_LIST;
	if (g_list_rows < 1)
		g_list_rows = 1;
}

int ui_init(void)
{
	initscr();
	if (LINES < 9 || COLS < 20) {
		endwin();
		return -1;
	}
	recompute_layout();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	set_escdelay(50);
	if (has_colors())
		init_colors();
	return 0;
}

void ui_cleanup(void)
{
	if (!isendwin()) {
		clear();
		refresh();
		endwin();
	}
}

void ui_set_title(const char *title)
{
	snprintf(g_title, sizeof(g_title), "%s", title ? title : "");
}

void ui_set_status(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(g_status, sizeof(g_status), fmt, ap);
	va_end(ap);
}

void ui_set_hints(const char *hints)
{
	snprintf(g_hints, sizeof(g_hints), "%s",
	         hints ? hints : "[j/k] Move  [Enter] Select  [q] Back");
}

void ui_set_items(const UiItem *items, int count)
{
	g_count = 0;
	if (items && count > 0) {
		g_count = (count > UI_MAX_ITEMS) ? UI_MAX_ITEMS : count;
		memcpy(g_items, items, (size_t)g_count * sizeof(UiItem));
	}
	if (g_cursor >= g_count)
		g_cursor = g_count > 0 ? g_count - 1 : 0;
	if (g_offset > g_cursor)
		g_offset = g_cursor;
	if (g_offset + g_list_rows <= g_cursor)
		g_offset = g_cursor - g_list_rows + 1;
	if (g_offset < 0)
		g_offset = 0;
}

void ui_set_cursor(int idx)
{
	if (idx < 0)
		idx = 0;
	if (g_count > 0 && idx >= g_count)
		idx = g_count - 1;
	g_cursor = idx;
}

int ui_get_cursor(void)
{
	return g_count > 0 ? g_cursor : -1;
}

int ui_get_selected_id(void)
{
	int i = ui_get_cursor();
	return i < 0 ? -1 : g_items[i].id;
}

void ui_draw(void)
{
	int i, row, label_w, val_w;

	recompute_layout();
	erase();

	attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
	mvhline(ROW_TITLE, 0, ' ', COLS);
	{
		int col = (COLS - (int)strlen(g_title)) / 2;
		if (col < 0)
			col = 0;
		mvprintw(ROW_TITLE, col, "%.*s", COLS, g_title);
	}
	attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

	attron(COLOR_PAIR(CP_STATUS));
	mvprintw(ROW_STATUS, 0, "%.*s", COLS, g_status);
	attroff(COLOR_PAIR(CP_STATUS));

	mvhline(ROW_DIV, 0, '-', COLS);

	for (i = 0; i < g_list_rows; i++) {
		int idx = g_offset + i;
		row = ROW_LIST + i;
		if (idx >= g_count)
			break;
		if (idx == g_cursor)
			attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
		mvhline(row, 0, ' ', COLS);
		if (g_items[idx].value[0]) {
			val_w = (int)strlen(g_items[idx].value);
			if (val_w > COLS / 2)
				val_w = COLS / 2;
			label_w = COLS - val_w - 2;
			if (label_w < 1)
				label_w = 1;
			mvprintw(row, 0, "%-*.*s", label_w, label_w, g_items[idx].label);
			mvprintw(row, COLS - val_w, "%.*s", val_w, g_items[idx].value);
		} else {
			mvprintw(row, 0, "%.*s", COLS, g_items[idx].label);
		}
		if (idx == g_cursor)
			attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
	}

	mvprintw(g_hint_row, 0, "%.*s", COLS, g_hints);
	refresh();
}

static void clamp_scroll(void)
{
	if (g_cursor < 0)
		g_cursor = 0;
	if (g_count > 0 && g_cursor >= g_count)
		g_cursor = g_count - 1;
	if (g_cursor < g_offset)
		g_offset = g_cursor;
	if (g_cursor >= g_offset + g_list_rows)
		g_offset = g_cursor - g_list_rows + 1;
	if (g_offset < 0)
		g_offset = 0;
}

int ui_get_key(void)
{
	int key;

	ui_draw();
	key = getch();

	switch (key) {
	case KEY_UP:
	case 'k':
		if (g_cursor > 0) {
			g_cursor--;
			clamp_scroll();
		}
		break;
	case KEY_DOWN:
	case 'j':
		if (g_count > 0 && g_cursor < g_count - 1) {
			g_cursor++;
			clamp_scroll();
		}
		break;
	case KEY_PPAGE:
		g_cursor -= g_list_rows;
		if (g_cursor < 0)
			g_cursor = 0;
		clamp_scroll();
		break;
	case KEY_NPAGE:
		g_cursor += g_list_rows;
		if (g_count > 0 && g_cursor >= g_count)
			g_cursor = g_count - 1;
		clamp_scroll();
		break;
	case KEY_HOME:
		g_cursor = 0;
		clamp_scroll();
		break;
	case KEY_END:
		g_cursor = g_count > 0 ? g_count - 1 : 0;
		clamp_scroll();
		break;
	case KEY_RESIZE:
		recompute_layout();
		clamp_scroll();
		break;
	default:
		break;
	}
	return key;
}

bool ui_confirm(const char *prompt)
{
	int box_w, box_h, box_y, box_x, hint_x;
	WINDOW *dlg;
	bool result = false;
	int key;

	box_w = (int)strlen(prompt) + 4;
	if (box_w < 16)
		box_w = 16;
	if (box_w > COLS - 2)
		box_w = COLS - 2;
	box_h = 5;
	box_y = (LINES - box_h) / 2;
	box_x = (COLS - box_w) / 2;

	dlg = newwin(box_h, box_w, box_y, box_x);
	if (!dlg)
		return false;
	keypad(dlg, TRUE);
	box(dlg, 0, 0);
	mvwprintw(dlg, 1, 2, "%.*s", box_w - 4, prompt);
	hint_x = (box_w - 12) / 2;
	if (hint_x < 1)
		hint_x = 1;
	wattron(dlg, A_BOLD);
	mvwprintw(dlg, 3, hint_x, "[Y]es  [N]o");
	wattroff(dlg, A_BOLD);
	wrefresh(dlg);
	flushinp();

	while (1) {
		key = wgetch(dlg);
		if (key == 'y' || key == 'Y') {
			result = true;
			break;
		}
		if (key == 'n' || key == 'N' || key == 27) {
			result = false;
			break;
		}
	}
	delwin(dlg);
	clear();
	ui_draw();
	return result;
}

int ui_prompt_text(const char *prompt, char *buf, int buf_len)
{
	int box_w, box_h, box_y, box_x, input_w, pos = 0, key;
	WINDOW *dlg;
	char input[128];

	if (!buf || buf_len < 2)
		return -1;

	box_w = COLS - 4;
	if (box_w < 20)
		box_w = COLS > 4 ? COLS - 2 : COLS;
	box_h = 6;
	box_y = (LINES - box_h) / 2;
	box_x = (COLS - box_w) / 2;
	input_w = box_w - 4;
	if (input_w > (int)sizeof(input) - 1)
		input_w = (int)sizeof(input) - 1;

	dlg = newwin(box_h, box_w, box_y, box_x);
	if (!dlg)
		return -1;
	keypad(dlg, TRUE);
	box(dlg, 0, 0);
	mvwprintw(dlg, 1, 2, "%.*s", box_w - 4, prompt);
	mvwprintw(dlg, 2, 2, "Enter to OK, Esc cancel");
	memset(input, 0, sizeof(input));
	curs_set(1);

	while (1) {
		wmove(dlg, 4, 2);
		for (int i = 0; i < input_w; i++)
			waddch(dlg, i < pos ? input[i] : ' ');
		wmove(dlg, 4, 2 + pos);
		wrefresh(dlg);
		key = wgetch(dlg);
		if (key == '\n' || key == KEY_ENTER)
			break;
		if (key == 27 || key == 'q') {
			curs_set(0);
			delwin(dlg);
			ui_draw();
			return -1;
		}
		if ((key == KEY_BACKSPACE || key == 127 || key == '\b') && pos > 0) {
			input[--pos] = '\0';
			continue;
		}
		if (key >= 32 && key <= 126 && pos < input_w && pos < buf_len - 1) {
			input[pos++] = (char)key;
			input[pos] = '\0';
		}
	}
	curs_set(0);
	delwin(dlg);
	snprintf(buf, (size_t)buf_len, "%s", input);
	ui_draw();
	return 0;
}

void ui_show_message(const char *msg, bool is_error, int duration_ms)
{
	int cp = is_error ? CP_ERROR : CP_OK;
	int mid = LINES / 2;
	int msg_len = (int)strlen(msg);
	int box_w = msg_len + 4;
	int box_x;

	if (box_w > COLS - 2)
		box_w = COLS - 2;
	box_x = (COLS - box_w) / 2;

	attron(COLOR_PAIR(cp) | A_BOLD);
	mvhline(mid, box_x, ' ', box_w);
	mvhline(mid + 1, box_x, ' ', box_w);
	mvhline(mid + 2, box_x, ' ', box_w);
	mvprintw(mid + 1, box_x + 2, "%.*s", box_w - 4, msg);
	attroff(COLOR_PAIR(cp) | A_BOLD);
	refresh();

	if (duration_ms > 0) {
		struct timespec ts = {
			.tv_sec = duration_ms / 1000,
			.tv_nsec = (duration_ms % 1000) * 1000000L
		};
		nanosleep(&ts, NULL);
	} else {
		flushinp();
		getch();
	}
	clear();
	ui_draw();
}

void ui_suspend(void)
{
	endwin();
}

int ui_resume(void)
{
	refresh();
	if (LINES < 9 || COLS < 20)
		return -1;
	recompute_layout();
	clear();
	ui_draw();
	return 0;
}
