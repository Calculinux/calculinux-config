#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

int system_get_hostname(char *out, size_t out_len);
int system_set_hostname(const char *name);

/* Change password via chpasswd. Returns 0 on success. */
int system_set_password(const char *user, const char *password);

int system_get_timezone(char *out, size_t out_len);
int system_set_timezone(const char *zone);

/* List top-level regions under /usr/share/zoneinfo (America, Europe, ...). */
int system_list_tz_regions(char names[][64], int max);
/* List zones under a region (e.g. America -> New_York). */
int system_list_tz_zones(const char *region, char names[][64], int max);

typedef struct {
	const char *unit;
	const char *label;
} KnownService;

extern const KnownService SYSTEM_KNOWN_SERVICES[];

bool system_unit_exists(const char *unit);
bool system_unit_enabled(const char *unit);
int  system_unit_set_enabled(const char *unit, bool enable);

#endif /* SYSTEM_H */
