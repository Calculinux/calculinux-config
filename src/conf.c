#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "conf.h"

static void trim_inplace(char *s)
{
	char *start = s;
	char *end;

	while (*start && isspace((unsigned char)*start))
		start++;
	if (start != s)
		memmove(s, start, strlen(start) + 1);
	end = s + strlen(s);
	while (end > s && isspace((unsigned char)end[-1]))
		*--end = '\0';
}

static void strip_comment(char *s)
{
	char *p = strchr(s, '#');
	if (p)
		*p = '\0';
}

bool conf_get(const char *path, const char *key, char *out, size_t out_len)
{
	FILE *f;
	char line[512];
	size_t klen;

	if (!path || !key || !out || out_len == 0)
		return false;
	out[0] = '\0';
	klen = strlen(key);
	f = fopen(path, "r");
	if (!f)
		return false;

	while (fgets(line, sizeof(line), f)) {
		char *eq, *val;
		trim_inplace(line);
		if (line[0] == '#' || line[0] == '\0')
			continue;
		eq = strchr(line, '=');
		if (!eq)
			continue;
		*eq = '\0';
		trim_inplace(line);
		if (strcmp(line, key) != 0)
			continue;
		val = eq + 1;
		trim_inplace(val);
		/* strip optional quotes */
		if ((val[0] == '"' || val[0] == '\'') && strlen(val) >= 2) {
			char q = val[0];
			size_t n = strlen(val);
			if (val[n - 1] == q) {
				val[n - 1] = '\0';
				val++;
			}
		}
		snprintf(out, out_len, "%s", val);
		fclose(f);
		(void)klen;
		return true;
	}
	fclose(f);
	return false;
}

int conf_set(const char *path, const char *key, const char *value)
{
	FILE *in, *out;
	char tmp[512];
	char line[512];
	int found = 0;
	int rc = -1;

	if (!path || !key || !value)
		return -1;

	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	in = fopen(path, "r");
	out = fopen(tmp, "w");
	if (!out) {
		if (in)
			fclose(in);
		return -1;
	}

	if (in) {
		while (fgets(line, sizeof(line), in)) {
			char copy[512];
			char *eq;
			snprintf(copy, sizeof(copy), "%s", line);
			/* keep comments and blanks as-is unless they are KEY= */
			{
				char check[512];
				snprintf(check, sizeof(check), "%s", line);
				trim_inplace(check);
				if (check[0] && check[0] != '#') {
					eq = strchr(check, '=');
					if (eq) {
						*eq = '\0';
						trim_inplace(check);
						if (strcmp(check, key) == 0) {
							fprintf(out, "%s=%s\n", key, value);
							found = 1;
							continue;
						}
					}
				}
			}
			fputs(line, out);
		}
		fclose(in);
	}

	if (!found)
		fprintf(out, "%s=%s\n", key, value);

	if (fclose(out) != 0)
		return -1;
	if (rename(tmp, path) != 0)
		return -1;
	rc = 0;
	return rc;
}

static void overlay_basename(const char *name, char *out, size_t out_len)
{
	const char *base = name;
	const char *slash = strrchr(name, '/');
	size_t n;

	if (slash)
		base = slash + 1;
	snprintf(out, out_len, "%s", base);
	n = strlen(out);
	if (n > 5 && strcmp(out + n - 5, ".dtbo") == 0)
		out[n - 5] = '\0';
}

bool overlay_conf_is_enabled(const char *path, const char *name)
{
	FILE *f;
	char line[512];
	char want[128];

	overlay_basename(name, want, sizeof(want));
	f = fopen(path, "r");
	if (!f)
		return false;

	while (fgets(line, sizeof(line), f)) {
		char entry[256];
		trim_inplace(line);
		if (line[0] == '#' || line[0] == '\0')
			continue;
		strip_comment(line);
		trim_inplace(line);
		if (!line[0])
			continue;
		overlay_basename(line, entry, sizeof(entry));
		if (strcmp(entry, want) == 0) {
			fclose(f);
			return true;
		}
	}
	fclose(f);
	return false;
}

int overlay_conf_set(const char *path, const char *name, bool enable)
{
	FILE *in, *out;
	char tmp[512];
	char line[512];
	char want[128];
	int found = 0;
	int wrote_enabled = 0;

	if (!path || !name)
		return -1;
	overlay_basename(name, want, sizeof(want));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);

	in = fopen(path, "r");
	out = fopen(tmp, "w");
	if (!out) {
		if (in)
			fclose(in);
		return -1;
	}

	if (in) {
		while (fgets(line, sizeof(line), in)) {
			char raw[512];
			char work[512];
			char entry[256];
			int was_comment = 0;
			char *p;

			snprintf(raw, sizeof(raw), "%s", line);
			snprintf(work, sizeof(work), "%s", line);
			trim_inplace(work);
			if (work[0] == '#') {
				was_comment = 1;
				p = work + 1;
				while (*p && isspace((unsigned char)*p))
					p++;
				memmove(work, p, strlen(p) + 1);
			}
			if (work[0] == '\0') {
				fputs(raw, out);
				continue;
			}
			{
				char stripped[512];
				snprintf(stripped, sizeof(stripped), "%s", work);
				strip_comment(stripped);
				trim_inplace(stripped);
				if (!stripped[0]) {
					fputs(raw, out);
					continue;
				}
				overlay_basename(stripped, entry, sizeof(entry));
			}
			if (strcmp(entry, want) != 0) {
				fputs(raw, out);
				continue;
			}
			found = 1;
			if (enable) {
				if (!wrote_enabled) {
					fprintf(out, "%s\n", want);
					wrote_enabled = 1;
				}
				/* drop duplicate commented/uncommented copies */
			} else {
				if (!was_comment)
					fprintf(out, "# %s\n", want);
				else
					fputs(raw, out);
			}
		}
		fclose(in);
	}

	if (enable && !wrote_enabled)
		fprintf(out, "%s\n", want);

	if (fclose(out) != 0)
		return -1;
	if (rename(tmp, path) != 0)
		return -1;
	(void)found;
	return 0;
}
