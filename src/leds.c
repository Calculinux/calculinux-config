#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"
#include "exec.h"
#include "leds.h"

int leds_list_triggers(const char *led_sysfs_dir, char names[][32], int max)
{
	char path[256];
	FILE *f;
	char buf[512];
	char *tok;
	int n = 0;

	if (!led_sysfs_dir || !names || max <= 0)
		return 0;
	snprintf(path, sizeof(path), "%s/trigger", led_sysfs_dir);
	f = fopen(path, "r");
	if (!f)
		return 0;
	if (!fgets(buf, sizeof(buf), f)) {
		fclose(f);
		return 0;
	}
	fclose(f);

	tok = strtok(buf, " \t\n");
	while (tok && n < max) {
		size_t len = strlen(tok);
		/* strip [active] markers */
		if (tok[0] == '[' && len > 2 && tok[len - 1] == ']') {
			tok[len - 1] = '\0';
			tok++;
		}
		snprintf(names[n], 32, "%s", tok);
		n++;
		tok = strtok(NULL, " \t\n");
	}
	return n;
}

int leds_get_trigger(const char *led_sysfs_dir, char *out, size_t out_len)
{
	char path[256];
	FILE *f;
	char buf[512];
	char *p, *end;

	if (!out || out_len == 0)
		return -1;
	out[0] = '\0';
	snprintf(path, sizeof(path), "%s/trigger", led_sysfs_dir);
	f = fopen(path, "r");
	if (!f)
		return -1;
	if (!fgets(buf, sizeof(buf), f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	p = strchr(buf, '[');
	if (!p)
		return -1;
	p++;
	end = strchr(p, ']');
	if (!end)
		return -1;
	*end = '\0';
	snprintf(out, out_len, "%s", p);
	return 0;
}

int leds_get_brightness(const char *sysfs_dir, char *out, size_t out_len)
{
	char path[256];
	FILE *f;

	if (!out || out_len == 0)
		return -1;
	out[0] = '\0';
	snprintf(path, sizeof(path), "%s/brightness", sysfs_dir);
	f = fopen(path, "r");
	if (!f)
		return -1;
	if (!fgets(out, (int)out_len, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	/* strip newline */
	{
		size_t n = strlen(out);
		while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r'))
			out[--n] = '\0';
	}
	return 0;
}

int leds_find_backlight(char *out, size_t out_len)
{
	DIR *d;
	struct dirent *e;

	if (!out || out_len == 0)
		return -1;
	out[0] = '\0';
	d = opendir("/sys/class/backlight");
	if (!d)
		return -1;
	while ((e = readdir(d)) != NULL) {
		if (e->d_name[0] == '.')
			continue;
		snprintf(out, out_len, "/sys/class/backlight/%s", e->d_name);
		closedir(d);
		return 0;
	}
	closedir(d);
	return -1;
}

int leds_save_and_apply(void)
{
	char *argv[] = { LEDS_APPLY_CMD, "apply", NULL };
	return exec_run(argv);
}
