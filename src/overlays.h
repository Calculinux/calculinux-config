#ifndef OVERLAYS_H
#define OVERLAYS_H

#include <stdbool.h>

#define OVERLAY_CONF_PATH "/etc/device-tree-overlays.conf"
#define OVERLAY_MAX       48
#define OVERLAY_NAME_MAX  64

typedef struct {
	char name[OVERLAY_NAME_MAX];
	bool enabled;
} OverlayEntry;

/* Scan /boot/devicetree and /lib/firmware/overlays for *.dtbo (deduped).
 * Fills entries with enabled state from OVERLAY_CONF_PATH. Returns count. */
int overlays_scan(OverlayEntry *entries, int max);

int overlays_toggle(const char *name);

#endif /* OVERLAYS_H */
