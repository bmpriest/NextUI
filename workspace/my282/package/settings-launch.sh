#!/bin/sh

cd "$(dirname "$0")" || exit 1
mkdir -p "$LOGS_PATH"
LOG_FILE="$LOGS_PATH/settings.txt"
if ( : > "$LOG_FILE" ) 2>/dev/null; then
	exec > "$LOG_FILE" 2>&1
else
	exec > /tmp/nextui-settings.txt 2>&1
fi
exec ./settings.elf
