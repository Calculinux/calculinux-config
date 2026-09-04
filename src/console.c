#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "console.h"
#include "exec.h"

int console_get_backend(char *out, size_t out_len)
{
	if (!conf_get(CONSOLE_DEFAULT_PATH, "CONSOLE", out, out_len)) {
		snprintf(out, out_len, "cruft");
		return 0;
	}
	if (strcmp(out, "yaft") == 0)
		snprintf(out, out_len, "cruft");
	return 0;
}

int console_get_font(char *out, size_t out_len)
{
	if (!conf_get(CONSOLE_DEFAULT_PATH, "CONSOLE_FONT", out, out_len))
		snprintf(out, out_len, "default");
	return 0;
}

int console_set_backend(const char *backend)
{
	return conf_set(CONSOLE_DEFAULT_PATH, "CONSOLE", backend);
}

int console_set_font(const char *font)
{
	return conf_set(CONSOLE_DEFAULT_PATH, "CONSOLE_FONT", font);
}

int console_apply(void)
{
	char *argv[] = { CONSOLE_MODE_CMD, "apply", NULL };
	return exec_run(argv);
}

const char *console_font_hint(const char *font)
{
	if (!font)
		return "";
	if (strcmp(font, "miniwi") == 0)
		return "~80x40";
	if (strcmp(font, "unifont") == 0)
		return "~40x20";
	return "~53x26";
}
