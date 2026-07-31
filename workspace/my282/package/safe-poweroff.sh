#!/bin/sh

# Run from /tmp so no executable, working directory, or inherited descriptor
# keeps the FAT SD card busy during final shutdown.

ACTION="${1:-poweroff}"
SDCARD_PATH="/mnt/SDCARD"

cd /tmp || exit 1

# Close descriptors inherited from the launch chain before inspecting other
# processes. Only system tools are used after this point.
export PATH=/usr/bin:/usr/sbin:/bin:/sbin
unset LD_LIBRARY_PATH

# SSH temporarily bind-mounts its development passwd file from the SD card.
# Release that mount explicitly so it cannot keep the card mounted.
SSH_STATE="$SDCARD_PATH/.userdata/my282/ssh"
if [ -f "$SSH_STATE/dropbear.pid" ]; then
	SSH_PID="$(cat "$SSH_STATE/dropbear.pid" 2>/dev/null)"
	case "$SSH_PID" in
		*[!0-9]*|'') ;;
		*) kill "$SSH_PID" 2>/dev/null ;;
	esac
fi
if [ -f "$SSH_STATE/passwd-mounted" ]; then
	umount /etc/passwd 2>/dev/null
fi
for fd in /proc/$$/fd/[3-9] /proc/$$/fd/[1-9][0-9]*; do
	[ -e "$fd" ] || continue
	eval "exec ${fd##*/}>&-" 2>/dev/null
done

# Stop any remaining process that still owns an SD-card file. A lazy unmount
# followed immediately by poweroff can leave FAT writes outstanding and was
# the likely source of the repeatable dirty/read-only mounts.
for pidpath in /proc/[0-9]*; do
	pid="${pidpath#/proc/}"
	[ "$pid" -gt 1 ] 2>/dev/null || continue
	[ "$pid" = "$$" ] && continue
	for fd in "$pidpath"/fd/*; do
		target="$(readlink "$fd" 2>/dev/null)" || continue
		case "$target" in
			"$SDCARD_PATH"/*)
				kill -9 "$pid" 2>/dev/null
				break
				;;
		esac
	done
done

killall wpa_supplicant 2>/dev/null
killall udhcpc 2>/dev/null

sleep 1
sync
SD_DEV="$(awk -v mp="$SDCARD_PATH" '$2 == mp {print $1; exit}' /proc/mounts)"
if [ -n "$SD_DEV" ]; then
	mount -o remount,ro "$SD_DEV" "$SDCARD_PATH" 2>/dev/null
fi
sync
umount "$SDCARD_PATH" 2>/dev/null

if [ "$ACTION" = "reboot" ]; then
	reboot
else
	poweroff
fi

while :; do
	sleep 10
done
