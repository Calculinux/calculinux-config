#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "exec.h"
#include "usb.h"

int usb_get(const char *key, char *out, size_t out_len)
{
	if (!conf_get(USB_DEFAULT_PATH, key, out, out_len)) {
		out[0] = '\0';
		return -1;
	}
	return 0;
}

int usb_set(const char *key, const char *value)
{
	return conf_set(USB_DEFAULT_PATH, key, value);
}

int usb_restart(void)
{
	char *argv[] = { "systemctl", "restart", USB_SERVICE, NULL };
	return exec_run(argv);
}
