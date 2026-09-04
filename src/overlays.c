#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "overlays.h"

static const char *OVERLAY_DIRS[] = {
	"/boot/devicetree",
	"/lib/firmware/overlays",
	NULL
};

static int already_have(OverlayEntry *entries, int count, const char *name)
{
	int i;
	for (i = 0; i < count; i++) {
		if (strcmp(entries[i].name, name) == 0)
			return 1;
	}
	return 0;
}

static void strip_dtbo(char *name)
{
	size_t n = strlen(name);
	if (n > 5 && strcmp(name + n - 5, ".dtbo") == 0)
		name[n - 5] = '\0';
}

int overlays_scan(OverlayEntry *entries, int max)
{
	int count = 0;
	int d;

	if (!entries || max <= 0)
		return 0;

	for (d = 0; OVERLAY_DIRS[d]; d++) {
		DIR *dir = opendir(OVERLAY_DIRS[d]);
		struct dirent *e;
		if (!dir)
			continue;
		while ((e = readdir(dir)) != NULL && count < max) {
			char name[OVERLAY_NAME_MAX];
			size_t n = strlen(e->d_name);
			if (n < 6 || n >= sizeof(name) ||
			    strcmp(e->d_name + n - 5, ".dtbo") != 0)
				continue;
			memcpy(name, e->d_name, n + 1);
			strip_dtbo(name);
			if (already_have(entries, count, name))
				continue;
			snprintf(entries[count].name, sizeof(entries[count].name),
			         "%s", name);
			entries[count].enabled =
				overlay_conf_is_enabled(OVERLAY_CONF_PATH, name);
			count++;
		}
		closedir(dir);
	}
	return count;
}

int overlays_toggle(const char *name)
{
	bool en = overlay_conf_is_enabled(OVERLAY_CONF_PATH, name);
	return overlay_conf_set(OVERLAY_CONF_PATH, name, !en);
}
