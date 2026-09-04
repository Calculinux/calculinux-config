#ifndef USB_H
#define USB_H

#include <stddef.h>

#define USB_DEFAULT_PATH "/etc/default/usb-gadget-network"
#define USB_SERVICE      "usb-gadget-network.service"

int usb_get(const char *key, char *out, size_t out_len);
int usb_set(const char *key, const char *value);
int usb_restart(void);

#endif /* USB_H */
