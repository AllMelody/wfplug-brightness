#!/bin/sh
# Restore saved LCD brightness early at boot.
# Usage: apply-brightness USERNAME
#
# Reads ~USERNAME/.config/wfplug-brightness/level and drives the PWM on
# /sys/class/pwm/pwmchip0/pwm0. Exits silently if no saved level exists.

set -e

user="${1:?usage: apply-brightness USERNAME}"
home=$(getent passwd "$user" | cut -d: -f6)
level_file="$home/.config/wfplug-brightness/level"

[ -r "$level_file" ] || exit 0

pct=$(cat "$level_file")
case "$pct" in
    ''|*[!0-9]*) exit 0 ;;
esac
[ "$pct" -ge 1 ] && [ "$pct" -le 100 ] || exit 0

chip=/sys/class/pwm/pwmchip0
chan=$chip/pwm0
period=1000000
duty=$(( pct * period / 100 ))

[ -d "$chip" ] || exit 0

# Display overlays declare a gpio-backlight node on GPIO 18 with
# default-on, which wins the probe race against dtoverlay=pwm and pins
# the GPIO high so PWM output never reaches the pin. Release the
# driver and remux GPIO 18 to PWM0 (ALT5) before configuring the PWM.
bl_drv=/sys/bus/platform/drivers/gpio-backlight
if [ -e "$bl_drv/rpi_backlight" ]; then
    echo rpi_backlight > "$bl_drv/unbind"
fi
pinctrl set 18 a5

if [ ! -d "$chan" ]; then
    echo 0 > "$chip/export"
    i=0
    while [ ! -w "$chan/period" ] && [ $i -lt 50 ]; do
        sleep 0.05
        i=$((i + 1))
    done
fi

echo "$period" > "$chan/period"
echo "$duty"   > "$chan/duty_cycle"
echo 1         > "$chan/enable"
