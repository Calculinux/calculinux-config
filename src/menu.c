#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

#include "conf.h"
#include "console.h"
#include "exec.h"
#include "leds.h"
#include "menu.h"
#include "overlays.h"
#include "system.h"
#include "ui.h"
#include "usb.h"

enum {
	ID_BACK = 0,
	ID_HW,
	ID_CONSOLE,
	ID_SYSTEM,
	ID_WIFI,
	ID_UPDATES,
	ID_OVERLAYS,
	ID_LEDS,
	ID_USB,
	ID_FONT,
	ID_BACKEND,
	ID_HOSTNAME,
	ID_PASS_PICO,
	ID_PASS_ROOT,
	ID_TIMEZONE,
	ID_SERVICES,
	ID_LED_LYRA_TRIG,
	ID_LED_LYRA_BRI,
	ID_LED_KBD_TRIG,
	ID_LED_KBD_BRI,
	ID_LED_LCD_BRI,
	ID_LED_APPLY,
	ID_USB_MODE,
	ID_USB_PROTO,
	ID_USB_SERIAL,
	ID_USB_NET,
	ID_USB_APPLY,
	ID_OVERLAY_BASE = 1000,
	ID_TRIG_BASE = 2000,
	ID_FONT_BASE = 3000,
	ID_BACKEND_BASE = 3100,
	ID_SVC_BASE = 3200,
	ID_TZ_REGION_BASE = 4000,
	ID_TZ_ZONE_BASE = 5000
};

static void item_set(UiItem *it, int id, const char *label, const char *value)
{
	it->id = id;
	snprintf(it->label, sizeof(it->label), "%s", label ? label : "");
	snprintf(it->value, sizeof(it->value), "%s", value ? value : "");
}

static int pick_list(const char *title, const char *status,
                     UiItem *items, int count)
{
	int key;

	ui_set_title(title);
	ui_set_status("%s", status ? status : "");
	ui_set_hints("[j/k] Move  [Enter] OK  [q] Back");
	ui_set_items(items, count);
	ui_set_cursor(0);

	while (1) {
		key = ui_get_key();
		if (key == 'q' || key == 27)
			return ID_BACK;
		if (key == '\n' || key == KEY_ENTER || key == '\r')
			return ui_get_selected_id();
	}
}

/* ── overlays ─────────────────────────────────────────────────────────── */

static void menu_overlays(void)
{
	OverlayEntry entries[OVERLAY_MAX];
	UiItem items[OVERLAY_MAX];
	int n, i, key;

	while (1) {
		n = overlays_scan(entries, OVERLAY_MAX);
		for (i = 0; i < n; i++) {
			item_set(&items[i], ID_OVERLAY_BASE + i, entries[i].name,
			         entries[i].enabled ? "ON" : "off");
		}
		ui_set_title("Overlays");
		ui_set_status(n ? "Toggle; reboot to apply" : "No .dtbo found");
		ui_set_hints("[Enter] Toggle  [q] Back");
		ui_set_items(items, n);

		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if ((key == '\n' || key == KEY_ENTER || key == '\r') && n > 0) {
			int idx = ui_get_cursor();
			if (idx >= 0 && overlays_toggle(entries[idx].name) == 0)
				ui_set_status("%s %s — reboot to apply",
				              entries[idx].name,
				              overlay_conf_is_enabled(OVERLAY_CONF_PATH,
				                                      entries[idx].name)
				                      ? "ON"
				                      : "off");
			else
				ui_show_message("Failed to update conf", true, 0);
		}
	}
}

/* ── LEDs ─────────────────────────────────────────────────────────────── */

static int pick_trigger(const char *led_dir, char *out, size_t out_len)
{
	char names[32][32];
	UiItem items[32];
	int n, i, id;

	n = leds_list_triggers(led_dir, names, 32);
	if (n <= 0) {
		ui_show_message("No triggers available", true, 0);
		return -1;
	}
	for (i = 0; i < n; i++)
		item_set(&items[i], ID_TRIG_BASE + i, names[i], "");
	id = pick_list("LED Trigger", led_dir, items, n);
	if (id < ID_TRIG_BASE)
		return -1;
	snprintf(out, out_len, "%s", names[id - ID_TRIG_BASE]);
	return 0;
}

static void menu_leds(void)
{
	UiItem items[8];
	char lyra_t[32], kbd_t[32], bri[32], bkl[128];
	int key, id;
	char input[32];

	while (1) {
		if (leds_get_trigger(LEDS_LYRA_PATH, lyra_t, sizeof(lyra_t)) < 0)
			conf_get(LEDS_DEFAULT_PATH, "LYRA_LED_TRIGGER", lyra_t, sizeof(lyra_t));
		if (leds_get_trigger(LEDS_KBD_PATH, kbd_t, sizeof(kbd_t)) < 0)
			conf_get(LEDS_DEFAULT_PATH, "KBD_LED_TRIGGER", kbd_t, sizeof(kbd_t));
		conf_get(LEDS_DEFAULT_PATH, "LYRA_LED_BRIGHTNESS", bri, sizeof(bri));

		item_set(&items[0], ID_LED_LYRA_TRIG, "Lyra trigger", lyra_t);
		{
			char v[24];
			conf_get(LEDS_DEFAULT_PATH, "LYRA_LED_BRIGHTNESS", v, sizeof(v));
			item_set(&items[1], ID_LED_LYRA_BRI, "Lyra bright", v[0] ? v : "-");
		}
		item_set(&items[2], ID_LED_KBD_TRIG, "Kbd trigger", kbd_t);
		{
			char v[24];
			conf_get(LEDS_DEFAULT_PATH, "KBD_LED_BRIGHTNESS", v, sizeof(v));
			item_set(&items[3], ID_LED_KBD_BRI, "Kbd bright", v[0] ? v : "-");
		}
		{
			char v[24];
			conf_get(LEDS_DEFAULT_PATH, "LCD_BRIGHTNESS", v, sizeof(v));
			if (!v[0] && leds_find_backlight(bkl, sizeof(bkl)) == 0)
				leds_get_brightness(bkl, v, sizeof(v));
			item_set(&items[4], ID_LED_LCD_BRI, "LCD bright", v[0] ? v : "-");
		}
		item_set(&items[5], ID_LED_APPLY, "Apply now", "");

		ui_set_title("LEDs");
		ui_set_status("Writes /etc/default/leds");
		ui_set_hints("[Enter] Edit  [q] Back");
		ui_set_items(items, 6);

		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if (!(key == '\n' || key == KEY_ENTER || key == '\r'))
			continue;
		id = ui_get_selected_id();

		if (id == ID_LED_LYRA_TRIG) {
			char t[32];
			if (pick_trigger(LEDS_LYRA_PATH, t, sizeof(t)) == 0)
				conf_set(LEDS_DEFAULT_PATH, "LYRA_LED_TRIGGER", t);
		} else if (id == ID_LED_KBD_TRIG) {
			char t[32];
			if (pick_trigger(LEDS_KBD_PATH, t, sizeof(t)) == 0)
				conf_set(LEDS_DEFAULT_PATH, "KBD_LED_TRIGGER", t);
		} else if (id == ID_LED_LYRA_BRI) {
			if (ui_prompt_text("Lyra brightness", input, sizeof(input)) == 0)
				conf_set(LEDS_DEFAULT_PATH, "LYRA_LED_BRIGHTNESS", input);
		} else if (id == ID_LED_KBD_BRI) {
			if (ui_prompt_text("Kbd brightness (empty=keep)", input, sizeof(input)) == 0)
				conf_set(LEDS_DEFAULT_PATH, "KBD_LED_BRIGHTNESS", input);
		} else if (id == ID_LED_LCD_BRI) {
			if (ui_prompt_text("LCD brightness (empty=keep)", input, sizeof(input)) == 0)
				conf_set(LEDS_DEFAULT_PATH, "LCD_BRIGHTNESS", input);
		} else if (id == ID_LED_APPLY) {
			if (leds_save_and_apply() == 0)
				ui_show_message("LEDs applied", false, 1000);
			else
				ui_show_message("Apply failed", true, 0);
		}
	}
}

/* ── USB ──────────────────────────────────────────────────────────────── */

static void menu_usb(void)
{
	UiItem items[8];
	char mode[32], proto[32], serial[8], net[8];
	int key, id;
	UiItem choices[4];
	int cid;

	while (1) {
		if (usb_get("USB_MODE", mode, sizeof(mode)) < 0)
			snprintf(mode, sizeof(mode), "gadget");
		if (usb_get("USB_PROTOCOL", proto, sizeof(proto)) < 0)
			snprintf(proto, sizeof(proto), "rndis");
		if (usb_get("ENABLE_SERIAL_CONSOLE", serial, sizeof(serial)) < 0)
			snprintf(serial, sizeof(serial), "0");
		if (usb_get("ENABLE_NETWORK", net, sizeof(net)) < 0)
			snprintf(net, sizeof(net), "1");

		item_set(&items[0], ID_USB_MODE, "Mode", mode);
		item_set(&items[1], ID_USB_PROTO, "Protocol", proto);
		item_set(&items[2], ID_USB_SERIAL, "Serial console",
		         serial[0] == '1' ? "ON" : "off");
		item_set(&items[3], ID_USB_NET, "USB network",
		         net[0] == '1' ? "ON" : "off");
		item_set(&items[4], ID_USB_APPLY, "Apply (restart)", "");

		ui_set_title("USB");
		ui_set_status("/etc/default/usb-gadget-network");
		ui_set_hints("[Enter] Edit  [q] Back");
		ui_set_items(items, 5);

		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if (!(key == '\n' || key == KEY_ENTER || key == '\r'))
			continue;
		id = ui_get_selected_id();

		if (id == ID_USB_MODE) {
			item_set(&choices[0], 1, "gadget", "");
			item_set(&choices[1], 2, "host", "");
			cid = pick_list("USB Mode", "", choices, 2);
			if (cid == 1)
				usb_set("USB_MODE", "gadget");
			else if (cid == 2)
				usb_set("USB_MODE", "host");
		} else if (id == ID_USB_PROTO) {
			item_set(&choices[0], 1, "rndis", "");
			item_set(&choices[1], 2, "ecm", "");
			item_set(&choices[2], 3, "both", "");
			cid = pick_list("USB Protocol", "", choices, 3);
			if (cid == 1)
				usb_set("USB_PROTOCOL", "rndis");
			else if (cid == 2)
				usb_set("USB_PROTOCOL", "ecm");
			else if (cid == 3)
				usb_set("USB_PROTOCOL", "both");
		} else if (id == ID_USB_SERIAL) {
			usb_set("ENABLE_SERIAL_CONSOLE",
			        serial[0] == '1' ? "0" : "1");
		} else if (id == ID_USB_NET) {
			usb_set("ENABLE_NETWORK", net[0] == '1' ? "0" : "1");
		} else if (id == ID_USB_APPLY) {
			if (usb_restart() == 0)
				ui_show_message("USB restarted", false, 1000);
			else
				ui_show_message("Restart failed", true, 0);
		}
	}
}

static void menu_hardware(void)
{
	UiItem items[4];
	int key, id;

	while (1) {
		item_set(&items[0], ID_OVERLAYS, "Overlays", "");
		item_set(&items[1], ID_LEDS, "LEDs", "");
		item_set(&items[2], ID_USB, "USB", "");
		ui_set_title("Hardware");
		ui_set_status("");
		ui_set_hints("[Enter] Select  [q] Back");
		ui_set_items(items, 3);
		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if (!(key == '\n' || key == KEY_ENTER || key == '\r'))
			continue;
		id = ui_get_selected_id();
		if (id == ID_OVERLAYS)
			menu_overlays();
		else if (id == ID_LEDS)
			menu_leds();
		else if (id == ID_USB)
			menu_usb();
	}
}

/* ── console ──────────────────────────────────────────────────────────── */

static void menu_console(void)
{
	UiItem items[4], choices[4];
	char backend[32], font[32];
	int key, id, cid;

	while (1) {
		console_get_backend(backend, sizeof(backend));
		console_get_font(font, sizeof(font));
		item_set(&items[0], ID_FONT, "Font", font);
		item_set(&items[1], ID_BACKEND, "Backend", backend);
		ui_set_title("Console");
		ui_set_status("Font: %s %s", font, console_font_hint(font));
		ui_set_hints("[Enter] Edit  [q] Back");
		ui_set_items(items, 2);

		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if (!(key == '\n' || key == KEY_ENTER || key == '\r'))
			continue;
		id = ui_get_selected_id();

		if (id == ID_FONT) {
			item_set(&choices[0], ID_FONT_BASE + 0, "default", "~53x26");
			item_set(&choices[1], ID_FONT_BASE + 1, "miniwi", "~80x40");
			item_set(&choices[2], ID_FONT_BASE + 2, "unifont", "~40x20");
			cid = pick_list("Font", "Logout VT after apply", choices, 3);
			if (cid == ID_FONT_BASE + 0)
				console_set_font("default");
			else if (cid == ID_FONT_BASE + 1)
				console_set_font("miniwi");
			else if (cid == ID_FONT_BASE + 2)
				console_set_font("unifont");
			else
				continue;
			console_apply();
			ui_show_message("Applied; log out of VT", false, 0);
		} else if (id == ID_BACKEND) {
			item_set(&choices[0], ID_BACKEND_BASE + 0, "cruft", "");
			item_set(&choices[1], ID_BACKEND_BASE + 1, "kernel", "");
			cid = pick_list("Backend", "", choices, 2);
			if (cid == ID_BACKEND_BASE + 0)
				console_set_backend("cruft");
			else if (cid == ID_BACKEND_BASE + 1)
				console_set_backend("kernel");
			else
				continue;
			console_apply();
			ui_show_message("Backend switched", false, 1000);
		}
	}
}

/* ── system ───────────────────────────────────────────────────────────── */

static void menu_services(void)
{
	UiItem items[8];
	const KnownService *s;
	int n, key, idx;
	bool enabled[8];

	while (1) {
		n = 0;
		for (s = SYSTEM_KNOWN_SERVICES; s->unit && n < 8; s++) {
			if (!system_unit_exists(s->unit))
				continue;
			enabled[n] = system_unit_enabled(s->unit);
			item_set(&items[n], ID_SVC_BASE + n, s->label,
			         enabled[n] ? "ON" : "off");
			n++;
		}
		ui_set_title("Services");
		ui_set_status(n ? "Toggle enable/disable" : "None installed");
		ui_set_hints("[Enter] Toggle  [q] Back");
		ui_set_items(items, n);

		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if ((key == '\n' || key == KEY_ENTER || key == '\r') && n > 0) {
			idx = ui_get_cursor();
			if (idx < 0 || idx >= n)
				continue;
			{
				int i = 0;
				for (s = SYSTEM_KNOWN_SERVICES; s->unit; s++) {
					if (!system_unit_exists(s->unit))
						continue;
					if (i == idx) {
						system_unit_set_enabled(s->unit, !enabled[idx]);
						break;
					}
					i++;
				}
			}
		}
	}
}

static void menu_timezone(void)
{
	char regions[64][64];
	char zones[128][64];
	UiItem items[128];
	int nr, nz, i, id;
	char region[64], zone[128];

	nr = system_list_tz_regions(regions, 64);
	if (nr <= 0) {
		ui_show_message("No zoneinfo found", true, 0);
		return;
	}
	for (i = 0; i < nr; i++)
		item_set(&items[i], ID_TZ_REGION_BASE + i, regions[i], "");
	id = pick_list("Timezone", "Pick region", items, nr);
	if (id < ID_TZ_REGION_BASE)
		return;
	snprintf(region, sizeof(region), "%s", regions[id - ID_TZ_REGION_BASE]);

	nz = system_list_tz_zones(region, zones, 128);
	if (nz <= 0) {
		/* region itself may be a zone file — try setting region */
		system_set_timezone(region);
		return;
	}
	for (i = 0; i < nz; i++)
		item_set(&items[i], ID_TZ_ZONE_BASE + i, zones[i], "");
	id = pick_list("Timezone", region, items, nz);
	if (id < ID_TZ_ZONE_BASE)
		return;
	snprintf(zone, sizeof(zone), "%s/%s", region, zones[id - ID_TZ_ZONE_BASE]);
	if (system_set_timezone(zone) == 0)
		ui_show_message(zone, false, 1000);
	else
		ui_show_message("Failed to set TZ", true, 0);
}

static void change_password(const char *user)
{
	char p1[64], p2[64];

	if (ui_prompt_text("New password", p1, sizeof(p1)) < 0)
		return;
	if (ui_prompt_text("Confirm password", p2, sizeof(p2)) < 0)
		return;
	if (strcmp(p1, p2) != 0) {
		ui_show_message("Passwords differ", true, 0);
		return;
	}
	if (system_set_password(user, p1) == 0)
		ui_show_message("Password updated", false, 1000);
	else
		ui_show_message("passwd failed", true, 0);
}

static void menu_system(void)
{
	UiItem items[8];
	char host[64], tz[64];
	int key, id;
	char newhost[64];

	while (1) {
		system_get_hostname(host, sizeof(host));
		if (system_get_timezone(tz, sizeof(tz)) < 0)
			snprintf(tz, sizeof(tz), "?");

		item_set(&items[0], ID_HOSTNAME, "Hostname", host);
		item_set(&items[1], ID_PASS_PICO, "Passwd pico", "");
		item_set(&items[2], ID_PASS_ROOT, "Passwd root", "");
		item_set(&items[3], ID_TIMEZONE, "Timezone", tz);
		item_set(&items[4], ID_SERVICES, "Services", "");

		ui_set_title("System");
		ui_set_status("%s", host);
		ui_set_hints("[Enter] Edit  [q] Back");
		ui_set_items(items, 5);

		key = ui_get_key();
		if (key == 'q' || key == 27)
			return;
		if (!(key == '\n' || key == KEY_ENTER || key == '\r'))
			continue;
		id = ui_get_selected_id();

		if (id == ID_HOSTNAME) {
			if (ui_prompt_text("Hostname", newhost, sizeof(newhost)) == 0) {
				if (system_set_hostname(newhost) == 0)
					ui_show_message("Hostname set", false, 1000);
				else
					ui_show_message("hostnamectl failed", true, 0);
			}
		} else if (id == ID_PASS_PICO) {
			change_password("pico");
		} else if (id == ID_PASS_ROOT) {
			change_password("root");
		} else if (id == ID_TIMEZONE) {
			menu_timezone();
		} else if (id == ID_SERVICES) {
			menu_services();
		}
	}
}

static void launch_tool(const char *path)
{
	char *argv[] = { (char *)path, NULL };
	int rc;

	ui_suspend();
	rc = exec_run(argv);
	if (ui_resume() < 0) {
		fprintf(stderr, "calculinux-config: terminal too small after %s\n", path);
		return;
	}
	if (rc == 127)
		ui_show_message("Not found", true, 0);
	else if (rc != 0)
		ui_set_status("%s exited %d", path, rc);
}

void menu_run(void)
{
	UiItem items[8];
	int key, id;

	while (1) {
		item_set(&items[0], ID_HW, "Hardware", "");
		item_set(&items[1], ID_CONSOLE, "Console", "");
		item_set(&items[2], ID_SYSTEM, "System", "");
		item_set(&items[3], ID_WIFI, "WiFi", "uwific");
		item_set(&items[4], ID_UPDATES, "Updates", "cup");

		ui_set_title("Calculinux Config");
		ui_set_status("PicoCalc system setup");
		ui_set_hints("[j/k] Move  [Enter] Select  [q] Quit");
		ui_set_items(items, 5);

		key = ui_get_key();
		if (key == 'q' || key == 27) {
			if (ui_confirm("Quit calculinux-config?"))
				return;
			continue;
		}
		if (!(key == '\n' || key == KEY_ENTER || key == '\r'))
			continue;
		id = ui_get_selected_id();
		if (id == ID_HW)
			menu_hardware();
		else if (id == ID_CONSOLE)
			menu_console();
		else if (id == ID_SYSTEM)
			menu_system();
		else if (id == ID_WIFI)
			launch_tool("uwific");
		else if (id == ID_UPDATES)
			launch_tool("cup");
	}
}
