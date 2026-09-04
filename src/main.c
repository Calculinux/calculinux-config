#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "menu.h"
#include "ui.h"

static void cleanup(void)
{
	ui_cleanup();
}

int main(void)
{
	atexit(cleanup);

	if (geteuid() != 0) {
		fprintf(stderr, "calculinux-config: must run as root (try sudo)\n");
		return EXIT_FAILURE;
	}

	if (ui_init() < 0) {
		fprintf(stderr, "calculinux-config: terminal too small (need 20x9)\n");
		return EXIT_FAILURE;
	}

	menu_run();
	return EXIT_SUCCESS;
}
