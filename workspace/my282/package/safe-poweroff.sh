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

# Stop any remaining process that owns the SD card. Checking descriptors alone
# is insufficient: Dropbear, keymon, and launcher children can retain an
# executable mapping or working directory on the card after their descriptors
# close, which leaves a read-only remount safe but makes the final unmount busy.
process_uses_sd() {
	pidpath="$1"

	for ref in cwd root exe; do
		target="$(readlink "$pidpath/$ref" 2>/dev/null)" || continue
		case "$target" in
			"$SDCARD_PATH"|"$SDCARD_PATH"/*) return 0 ;;
		esac
	done

	for fd in "$pidpath"/fd/*; do
		target="$(readlink "$fd" 2>/dev/null)" || continue
		case "$target" in
			"$SDCARD_PATH"|"$SDCARD_PATH"/*) return 0 ;;
		esac
	done

	grep -q " $SDCARD_PATH/" "$pidpath/maps" 2>/dev/null && return 0
	return 1
}

stop_sd_users() {
	for pidpath in /proc/[0-9]*; do
		pid="${pidpath#/proc/}"
		[ "$pid" -gt 1 ] 2>/dev/null || continue
		[ "$pid" = "$$" ] && continue
		if process_uses_sd "$pidpath"; then
			kill -9 "$pid" 2>/dev/null
		fi
	done
}

stop_sd_users

killall wpa_supplicant 2>/dev/null
killall udhcpc 2>/dev/null

sleep 1
sync
SD_DEV="$(awk -v mp="$SDCARD_PATH" '$2 == mp {print $1; exit}' /proc/mounts)"
SD_READONLY=0
if [ -n "$SD_DEV" ]; then
	mount -o remount,ro "$SD_DEV" "$SDCARD_PATH" 2>/dev/null
	SD_OPTIONS="$(awk -v mp="$SDCARD_PATH" '$2 == mp {print $4; exit}' /proc/mounts)"
	case ",$SD_OPTIONS," in
		*,ro,*) SD_READONLY=1 ;;
	esac
fi
sync
if ! umount "$SDCARD_PATH" 2>/dev/null; then
	# A process that was exiting during the first pass may have become visible
	# only after the remount. Sweep once more and retry by block device because
	# this A30 BusyBox also requires explicit device arguments for remounts.
	sleep 1
	stop_sd_users
	sync
	if ! umount "$SD_DEV" 2>/dev/null; then
		# Spruce uses the same final A30 fallback. Lazy detach is safe here only
		# because sync and the verified read-only remount prohibit later writes.
		if [ "$SD_READONLY" -eq 1 ]; then
			umount -l "$SDCARD_PATH" 2>/dev/null
		fi
	fi
fi

if [ "$ACTION" = "reboot" ]; then
	reboot
else
	poweroff
fi

while :; do
	sleep 10
done
