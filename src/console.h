#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

#define CONSOLE_DEFAULT_PATH "/etc/default/console"
#define CONSOLE_MODE_CMD     "/usr/bin/console-mode"

int console_get_backend(char *out, size_t out_len);
int console_get_font(char *out, size_t out_len);
int console_set_backend(const char *backend);
int console_set_font(const char *font);
int console_apply(void);

/* Human-readable grid hint for a font profile. */
const char *console_font_hint(const char *font);

#endif /* CONSOLE_H */
