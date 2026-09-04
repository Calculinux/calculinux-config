#!/bin/bash
# Self-check: overlay conf edit + calculinux-leds against a fake sysfs tree.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

assert_eq() {
	[ "$1" = "$2" ] || { echo "FAIL: expected '$2' got '$1' ($3)" >&2; exit 1; }
}

assert_file_contains() {
	grep -qF "$2" "$1" || { echo "FAIL: '$1' missing '$2'" >&2; exit 1; }
}

assert_file_not_contains_active() {
	# active (uncommented) line with exact name
	if grep -E "^[[:space:]]*$2([[:space:]]|$)" "$1" >/dev/null; then
		echo "FAIL: '$1' still has active '$2'" >&2
		exit 1
	fi
}

# --- overlay conf ---
CONF="$TMP/device-tree-overlays.conf"
cat >"$CONF" <<'EOF'
# List of device tree overlays.
# ds3231-rtc
# sx1262-lora
EOF

# Build a tiny C helper that links only conf.o — or exercise via a host compile of conf.
# Prefer compiling a minimal harness against conf.c.
cat >"$TMP/overlay_harness.c" <<'EOF'
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf.h"

int main(int argc, char **argv)
{
	const char *path, *name, *op;
	if (argc != 4) {
		fprintf(stderr, "usage: %s <conf> <name> enable|disable|check\n", argv[0]);
		return 2;
	}
	path = argv[1];
	name = argv[2];
	op = argv[3];
	if (strcmp(op, "check") == 0)
		return overlay_conf_is_enabled(path, name) ? 0 : 1;
	if (strcmp(op, "enable") == 0)
		return overlay_conf_set(path, name, 1) == 0 ? 0 : 1;
	if (strcmp(op, "disable") == 0)
		return overlay_conf_set(path, name, 0) == 0 ? 0 : 1;
	return 2;
}
EOF

cc -std=c11 -Wall -Wextra -I"$ROOT/src" -o "$TMP/overlay_harness" \
	"$TMP/overlay_harness.c" "$ROOT/src/conf.c"

"$TMP/overlay_harness" "$CONF" ds3231-rtc check && { echo "FAIL: should be disabled"; exit 1; }
"$TMP/overlay_harness" "$CONF" ds3231-rtc enable
"$TMP/overlay_harness" "$CONF" ds3231-rtc check
assert_file_contains "$CONF" "ds3231-rtc"
# ensure not only commented
grep -E '^[[:space:]]*ds3231-rtc([[:space:]]|$)' "$CONF" >/dev/null

"$TMP/overlay_harness" "$CONF" ds3231-rtc disable
"$TMP/overlay_harness" "$CONF" ds3231-rtc check && { echo "FAIL: should be disabled after toggle"; exit 1; }
assert_file_not_contains_active "$CONF" "ds3231-rtc"

# enable absolute-style name with .dtbo suffix
"$TMP/overlay_harness" "$CONF" sx1262-lora.dtbo enable
"$TMP/overlay_harness" "$CONF" sx1262-lora check

# conf_set / conf_get
cat >"$TMP/kv" <<'EOF'
# comment
FOO=bar
BAZ=qux
EOF
cat >"$TMP/kv_harness.c" <<'EOF'
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf.h"
int main(void) {
	char buf[64];
	const char *kv = getenv("KV");
	if (!kv) return 9;
	if (!conf_get(kv, "FOO", buf, sizeof(buf))) return 1;
	if (strcmp(buf, "bar") != 0) return 2;
	if (conf_set(kv, "FOO", "xyz") != 0) return 3;
	if (!conf_get(kv, "FOO", buf, sizeof(buf))) return 4;
	if (strcmp(buf, "xyz") != 0) return 5;
	if (conf_set(kv, "NEW", "1") != 0) return 6;
	if (!conf_get(kv, "NEW", buf, sizeof(buf))) return 7;
	if (strcmp(buf, "1") != 0) return 8;
	return 0;
}
EOF
cc -std=c11 -Wall -Wextra -I"$ROOT/src" -o "$TMP/kv_harness" \
	"$TMP/kv_harness.c" "$ROOT/src/conf.c"
KV="$TMP/kv" "$TMP/kv_harness"

# --- calculinux-leds with fake sysfs ---
SYS="$TMP/sys"
mkdir -p "$SYS/class/leds/luckfox_lyra:user_led"
mkdir -p "$SYS/class/leds/picocalc_mfd_led:kbd_backlight"
mkdir -p "$SYS/class/backlight/picocalc-mfd-bkl"
printf 'none timer heartbeat [default-on]\n' >"$SYS/class/leds/luckfox_lyra:user_led/trigger"
printf '1\n' >"$SYS/class/leds/luckfox_lyra:user_led/brightness"
printf 'none [backlight] timer\n' >"$SYS/class/leds/picocalc_mfd_led:kbd_backlight/trigger"
printf '80\n' >"$SYS/class/leds/picocalc_mfd_led:kbd_backlight/brightness"
printf '128\n' >"$SYS/class/backlight/picocalc-mfd-bkl/brightness"
printf '255\n' >"$SYS/class/backlight/picocalc-mfd-bkl/max_brightness"

LEDS="$TMP/leds.default"
cat >"$LEDS" <<'EOF'
LYRA_LED_TRIGGER=none
LYRA_LED_BRIGHTNESS=0
KBD_LED_TRIGGER=timer
KBD_LED_BRIGHTNESS=40
LCD_BRIGHTNESS=200
EOF

chmod +x "$ROOT/scripts/calculinux-leds"
SYSFS_ROOT="$SYS" LEDS_DEFAULT="$LEDS" "$ROOT/scripts/calculinux-leds" apply

assert_eq "$(tr -d '\n' <"$SYS/class/leds/luckfox_lyra:user_led/trigger")" "none" "lyra trigger"
assert_eq "$(tr -d '\n' <"$SYS/class/leds/luckfox_lyra:user_led/brightness")" "0" "lyra brightness"
assert_eq "$(tr -d '\n' <"$SYS/class/leds/picocalc_mfd_led:kbd_backlight/trigger")" "timer" "kbd trigger"
assert_eq "$(tr -d '\n' <"$SYS/class/leds/picocalc_mfd_led:kbd_backlight/brightness")" "40" "kbd brightness"
assert_eq "$(tr -d '\n' <"$SYS/class/backlight/picocalc-mfd-bkl/brightness")" "200" "lcd brightness"

# empty brightness leaves hardware alone
cat >"$LEDS" <<'EOF'
LYRA_LED_TRIGGER=heartbeat
LYRA_LED_BRIGHTNESS=
KBD_LED_TRIGGER=
KBD_LED_BRIGHTNESS=
LCD_BRIGHTNESS=
EOF
printf '99\n' >"$SYS/class/leds/luckfox_lyra:user_led/brightness"
SYSFS_ROOT="$SYS" LEDS_DEFAULT="$LEDS" "$ROOT/scripts/calculinux-leds" apply
assert_eq "$(tr -d '\n' <"$SYS/class/leds/luckfox_lyra:user_led/trigger")" "heartbeat" "lyra heartbeat"
assert_eq "$(tr -d '\n' <"$SYS/class/leds/luckfox_lyra:user_led/brightness")" "99" "lyra brightness preserved"

echo "OK: all checks passed"
