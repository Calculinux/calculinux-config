#ifndef UI_H
#define UI_H

#include <stdbool.h>

/*
 * Compact ncurses chrome for PicoCalc (~40x20 unifont, ~53x26 default).
 *
 *   Row 0   title bar
 *   Row 1   status line
 *   Row 2   divider
 *   Row 3+  scrollable list
 *   Last    key hints
 */

#define UI_ITEM_LABEL_MAX 40
#define UI_ITEM_VALUE_MAX 24
#define UI_MAX_ITEMS      64

typedef struct {
	char label[UI_ITEM_LABEL_MAX];
	char value[UI_ITEM_VALUE_MAX]; /* shown right-aligned when non-empty */
	int  id;
} UiItem;

int  ui_init(void);
void ui_cleanup(void);

void ui_set_title(const char *title);
void ui_set_status(const char *fmt, ...);
void ui_set_hints(const char *hints);
void ui_set_items(const UiItem *items, int count);
void ui_set_cursor(int idx);
int  ui_get_cursor(void);
int  ui_get_selected_id(void);

void ui_draw(void);

/* Blocks for a key. Handles j/k/arrows for list move and KEY_RESIZE.
 * Returns the raw key (Enter, Esc, 'q', letter shortcuts, etc.). */
int  ui_get_key(void);

bool ui_confirm(const char *prompt);
/* Text prompt. Returns 0 on OK, -1 on cancel. */
int  ui_prompt_text(const char *prompt, char *buf, int buf_len);
void ui_show_message(const char *msg, bool is_error, int duration_ms);

/* Suspend/resume around fork+exec of another full-screen program. */
void ui_suspend(void);
int  ui_resume(void);

#endif /* UI_H */
