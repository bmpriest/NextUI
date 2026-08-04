#!/bin/sh

DIR="$(dirname "$0")"
STATE="$USERDATA_PATH/files"
LOG="$LOGS_PATH/files.txt"

mkdir -p "$STATE" "$LOGS_PATH"
cd "$DIR" || exit 1

# Development-only tracing is opt-in so release behavior and performance are
# unchanged. Keep the trace in RAM: a device reset must not leave a partially
# written FAT file behind.
if [ -f "$STATE/trace" ] && command -v strace >/dev/null 2>&1; then
	rm -f /tmp/nextcommander.strace
	exec strace -f -tt -s 256 -o /tmp/nextcommander.strace \
		./NextCommander --config my282.cfg --res-dir res > "$LOG" 2>&1
fi

exec ./NextCommander --config my282.cfg --res-dir res > "$LOG" 2>&1
