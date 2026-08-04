#!/bin/sh

DIR="$(dirname "$0")"
STATE="$USERDATA_PATH/ssh"
LOG="$LOGS_PATH/ssh.txt"
PID_FILE="$STATE/dropbear.pid"
PASSWD_MARKER="$STATE/passwd-mounted"

mkdir -p "$STATE" "$LOGS_PATH"

log() {
	echo "$*" >> "$LOG"
}

is_dropbear_pid() {
	PID="$1"
	case "$PID" in
		*[!0-9]*|'') return 1 ;;
	esac
	[ -e "/proc/$PID/exe" ] || return 1
	case "$(readlink "/proc/$PID/exe" 2>/dev/null)" in
		*/dropbearmulti) return 0 ;;
	esac
	return 1
}

stop_ssh() {
	if [ -f "$PID_FILE" ]; then
		PID="$(cat "$PID_FILE" 2>/dev/null)"
		if is_dropbear_pid "$PID"; then
			kill "$PID" 2>/dev/null
		fi
		rm -f "$PID_FILE"
	fi

	if [ -f "$PASSWD_MARKER" ]; then
		umount /etc/passwd 2>/dev/null
		rm -f "$PASSWD_MARKER"
	fi
	log "SSH stopped"
}

if [ -f "$PID_FILE" ]; then
	PID="$(cat "$PID_FILE" 2>/dev/null)"
	if is_dropbear_pid "$PID"; then
		stop_ssh
		exit 0
	fi
	log "Discarding stale SSH pid file ($PID)"
	rm -f "$PID_FILE"
fi

: > "$LOG"

IP=""
ATTEMPTS=0
while [ "$ATTEMPTS" -lt 15 ]; do
	IP="$(ifconfig wlan0 2>/dev/null | sed -n 's/.*inet addr:\([^ ]*\).*/\1/p; s/.*inet \([^ ]*\).*/\1/p' | head -n 1)"
	[ -n "$IP" ] && break
	ATTEMPTS=$((ATTEMPTS + 1))
	sleep 1
done
if [ -z "$IP" ]; then
	log "Wi-Fi has no IP address; connect in Settings > WiFi first"
	exit 1
fi

# Provide one development-only account without altering the firmware file.
if mount -o bind "$DIR/passwd" /etc/passwd 2>> "$LOG"; then
	touch "$PASSWD_MARKER"
else
	log "Could not bind the SSH passwd file"
	exit 1
fi

if [ ! -f "$STATE/dropbear_ed25519_host_key" ]; then
	"$DIR/dropbearmulti" dropbearkey -t ed25519 \
		-f "$STATE/dropbear_ed25519_host_key" >> "$LOG" 2>&1 || {
			stop_ssh
			exit 1
		}
fi

"$DIR/dropbearmulti" dropbear \
	-r "$STATE/dropbear_ed25519_host_key" \
	-p 22 -P "$PID_FILE" -c "$DIR/shell-wrapper.sh" >> "$LOG" 2>&1

sleep 1
if [ ! -f "$PID_FILE" ]; then
	log "Dropbear failed to start (is port 22 already in use?)"
	stop_ssh
	exit 1
fi

log "SSH started${IP:+ at $IP}:22"
log "Login: spruce / happygaming"
exit 0
