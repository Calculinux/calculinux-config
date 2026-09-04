#ifndef CONF_H
#define CONF_H

#include <stdbool.h>
#include <stddef.h>

/* Read KEY=VALUE from a shell-style defaults file into out (NUL-terminated).
 * Returns true if found. Empty values are valid (out[0] == '\0'). */
bool conf_get(const char *path, const char *key, char *out, size_t out_len);

/* Set or replace KEY=VALUE. Creates the file if missing. Preserves other
 * lines and comments. Returns 0 on success. */
int conf_set(const char *path, const char *key, const char *value);

/* Overlay list helpers for /etc/device-tree-overlays.conf style files. */
bool overlay_conf_is_enabled(const char *path, const char *name);
/* Enable: ensure an uncommented line exists. Disable: comment it out. */
int  overlay_conf_set(const char *path, const char *name, bool enable);

#endif /* CONF_H */
