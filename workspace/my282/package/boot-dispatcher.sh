#!/bin/sh

INFO="$(cat /proc/cpuinfo 2>/dev/null)"
case "$INFO" in
*"sun8i"*)
	if [ -d /usr/miyoo ]; then
		exec /mnt/SDCARD/.tmp_update/my282.sh
	fi
	;;
esac

echo "This NextUI image supports the Miyoo A30 only." >&2
sync
poweroff
