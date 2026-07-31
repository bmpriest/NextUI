#!/bin/sh

SDCARD_PATH="/mnt/SDCARD"
SYSTEM_PATH="$SDCARD_PATH/.system/my282"
LAUNCH_PATH="$SYSTEM_PATH/paks/MinUI.pak/launch.sh"
SAFE_POWEROFF_SOURCE="$SYSTEM_PATH/bin/safe-poweroff.sh"
SAFE_POWEROFF="/tmp/nextui-safe-poweroff.sh"

export PATH="$SYSTEM_PATH/bin:/usr/miyoo/bin:/usr/miyoo/sbin:/usr/bin:/usr/sbin:/bin:/sbin"
export LD_LIBRARY_PATH="$SYSTEM_PATH/lib:/usr/miyoo/lib:/usr/lib:/lib"

cp "$SAFE_POWEROFF_SOURCE" "$SAFE_POWEROFF"
chmod 755 "$SAFE_POWEROFF"

# A30 stock firmware may mount an unclean FAT card read-only. BusyBox mount on
# this firmware needs both the block device and mountpoint for a remount; using
# only the mountpoint silently left the card read-only on two separate cards.
RW_TEST="$SDCARD_PATH/.nextui-rw-test"
if ! touch "$RW_TEST" 2>/dev/null; then
	SD_DEV="$(awk -v mp="$SDCARD_PATH" '$2 == mp {print $1; exit}' /proc/mounts)"
	if [ -n "$SD_DEV" ]; then
		mount -o remount,rw "$SD_DEV" "$SDCARD_PATH" 2>/dev/null
	fi
fi
touch "$RW_TEST" 2>/dev/null && rm -f "$RW_TEST"

# The stock supervisor can remove the update hook after it returns. Keep it
# out of the standalone session, along with the respawning USB MTP daemon.
killall -9 main 2>/dev/null
while :; do
	killall -9 MtpDaemon 2>/dev/null
	sleep 5
done &
MTP_KILLER=$!

echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 0 > /sys/class/leds/led1/brightness 2>/dev/null

if [ -f "$LAUNCH_PATH" ]; then
	"$LAUNCH_PATH"
fi

kill "$MTP_KILLER" 2>/dev/null
sync
ACTION=poweroff
if [ -f /tmp/reboot ]; then
	ACTION=reboot
fi
exec "$SAFE_POWEROFF" "$ACTION"
