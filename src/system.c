#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "exec.h"
#include "system.h"

const KnownService SYSTEM_KNOWN_SERVICES[] = {
	{ "usb-gadget-network.service",        "USB gadget net" },
	{ "usb-gadget-serial-console.service", "USB serial" },
	{ "zerotier-one.service",              "ZeroTier" },
	{ "meshtasticd.service",               "Meshtastic" },
	{ NULL, NULL }
};

int system_get_hostname(char *out, size_t out_len)
{
	char *argv[] = { "hostnamectl", "--static", NULL };
	char buf[128];
	int rc;

	if (!out || out_len == 0)
		return -1;
	rc = exec_capture(argv, buf, sizeof(buf));
	if (rc != 0) {
		/* fallback */
		if (gethostname(out, out_len) != 0) {
			out[0] = '\0';
			return -1;
		}
		out[out_len - 1] = '\0';
		return 0;
	}
	{
		size_t n = strlen(buf);
		while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
			buf[--n] = '\0';
	}
	snprintf(out, out_len, "%s", buf);
	return 0;
}

int system_set_hostname(const char *name)
{
	char *argv[] = { "hostnamectl", "set-hostname", (char *)name, NULL };
	return exec_run(argv);
}

int system_set_password(const char *user, const char *password)
{
	int pipefd[2];
	pid_t pid;
	int status;
	char line[256];

	if (!user || !password)
		return -1;
	if (pipe(pipefd) < 0)
		return -1;

	pid = fork();
	if (pid < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}
	if (pid == 0) {
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		execlp("chpasswd", "chpasswd", (char *)NULL);
		_exit(127);
	}
	close(pipefd[0]);
	snprintf(line, sizeof(line), "%s:%s\n", user, password);
	{
		size_t len = strlen(line);
		ssize_t w = write(pipefd[1], line, len);
		(void)w;
	}
	close(pipefd[1]);
	if (waitpid(pid, &status, 0) < 0)
		return -1;
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

int system_get_timezone(char *out, size_t out_len)
{
	char *argv[] = { "timedatectl", "show", "-p", "Timezone", "--value", NULL };
	char buf[128];
	int rc;

	if (!out || out_len == 0)
		return -1;
	rc = exec_capture(argv, buf, sizeof(buf));
	if (rc != 0) {
		out[0] = '\0';
		return -1;
	}
	{
		size_t n = strlen(buf);
		while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
			buf[--n] = '\0';
	}
	snprintf(out, out_len, "%s", buf);
	return 0;
}

int system_set_timezone(const char *zone)
{
	char *argv[] = { "timedatectl", "set-timezone", (char *)zone, NULL };
	return exec_run(argv);
}

static int is_skip_tz_name(const char *name)
{
	static const char *skip[] = {
		"posix", "right", "Etc", "localtime", "leapseconds",
		"tzdata.zi", "zone.tab", "zone1970.tab", "iso3166.tab",
		"zonenow.tab", NULL
	};
	int i;
	for (i = 0; skip[i]; i++) {
		if (strcmp(name, skip[i]) == 0)
			return 1;
	}
	return 0;
}

int system_list_tz_regions(char names[][64], int max)
{
	DIR *d;
	struct dirent *e;
	int n = 0;

	d = opendir("/usr/share/zoneinfo");
	if (!d)
		return 0;
	while ((e = readdir(d)) != NULL && n < max) {
		char path[320];
		size_t namelen;
		if (e->d_name[0] == '.')
			continue;
		if (is_skip_tz_name(e->d_name))
			continue;
		namelen = strlen(e->d_name);
		if (namelen == 0 || namelen >= 64)
			continue;
		snprintf(path, sizeof(path), "/usr/share/zoneinfo/%s", e->d_name);
		{
			DIR *sub = opendir(path);
			if (!sub)
				continue;
			closedir(sub);
		}
		memcpy(names[n], e->d_name, namelen + 1);
		n++;
	}
	closedir(d);
	return n;
}

int system_list_tz_zones(const char *region, char names[][64], int max)
{
	DIR *d;
	struct dirent *e;
	char path[256];
	int n = 0;

	if (!region)
		return 0;
	snprintf(path, sizeof(path), "/usr/share/zoneinfo/%s", region);
	d = opendir(path);
	if (!d)
		return 0;
	while ((e = readdir(d)) != NULL && n < max) {
		size_t namelen;
		if (e->d_name[0] == '.')
			continue;
		namelen = strlen(e->d_name);
		if (namelen == 0 || namelen >= 64)
			continue;
		memcpy(names[n], e->d_name, namelen + 1);
		n++;
	}
	closedir(d);
	return n;
}

bool system_unit_exists(const char *unit)
{
	char *argv[] = { "systemctl", "cat", (char *)unit, NULL };
	char buf[64];
	return exec_capture(argv, buf, sizeof(buf)) == 0;
}

bool system_unit_enabled(const char *unit)
{
	char *argv[] = { "systemctl", "is-enabled", (char *)unit, NULL };
	char buf[64];
	int rc = exec_capture(argv, buf, sizeof(buf));
	(void)rc;
	return strncmp(buf, "enabled", 7) == 0;
}

int system_unit_set_enabled(const char *unit, bool enable)
{
	char *argv_en[] = { "systemctl", "enable", "--now", (char *)unit, NULL };
	char *argv_dis[] = { "systemctl", "disable", "--now", (char *)unit, NULL };
	return exec_run(enable ? argv_en : argv_dis);
}
