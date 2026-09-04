#ifndef LEDS_H
#define LEDS_H

#include <stddef.h>

#define LEDS_DEFAULT_PATH "/etc/default/leds"
#define LEDS_APPLY_CMD    "/usr/sbin/calculinux-leds"

#define LEDS_LYRA_PATH "/sys/class/leds/luckfox_lyra:user_led"
#define LEDS_KBD_PATH  "/sys/class/leds/picocalc_mfd_led:kbd_backlight"

/* Parse space-separated trigger list from sysfs trigger file.
 * The active trigger is marked [name]. Fills names[], returns count. */
int leds_list_triggers(const char *led_sysfs_dir, char names[][32], int max);

/* Read currently active trigger into out. */
int leds_get_trigger(const char *led_sysfs_dir, char *out, size_t out_len);

int leds_get_brightness(const char *sysfs_dir, char *out, size_t out_len);
int leds_find_backlight(char *out, size_t out_len);

int leds_save_and_apply(void);

#endif /* LEDS_H */
