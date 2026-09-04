# Calculinux Config

Small-screen ncurses system setup for Calculinux on PicoCalc (320x320).

```
sudo calculinux-config
# or: sudo ccfg
```

## Menus

| Section | Actions |
|---------|---------|
| Hardware / Overlays | Toggle `/etc/device-tree-overlays.conf` (reboot to apply) |
| Hardware / LEDs | Lyra LED, keyboard backlight, LCD brightness via `/etc/default/leds` |
| Hardware / USB | Edit `/etc/default/usb-gadget-network` and restart the service |
| Console | Font (`default`/`miniwi`/`unifont`) and backend (`cruft`/`kernel`) |
| System | Hostname, passwords, timezone, known services |
| WiFi | Launches `uwific` |
| Updates | Launches `cup` |

Designed for ~40x20 (unifont) and ~53x26 (default cruft font).

## Build

```bash
make
sudo make install
make check
```

Requires `ncurses` (`pkg-config ncurses`).

## LEDs

`/etc/default/leds` is applied by `calculinux-leds apply` and by
`calculinux-leds.service` at boot. DT defaults (`heartbeat` / `backlight`)
still apply for early boot before systemd.
