#!/bin/sh

PROBE_DIR="/mnt/SDCARD/NextUI-my282"
REPORT="$PROBE_DIR/my282-probe.txt"
MAINUI_PID="$(pidof MainUI 2>/dev/null)"

resume_mainui() {
    if [ -n "$MAINUI_PID" ]; then
        kill -CONT "$MAINUI_PID" 2>/dev/null
    fi
}

trap resume_mainui EXIT HUP INT TERM

if [ -n "$MAINUI_PID" ]; then
    kill -STOP "$MAINUI_PID"
fi

cd "$PROBE_DIR" || exit 1
export LD_LIBRARY_PATH="/mnt/SDCARD/spruce/a30/lib:/mnt/SDCARD/miyoo/lib:/usr/miyoo/lib:/usr/lib:/lib"
export SDL_VIDEODRIVER=mali

./my282-probe.elf > "$REPORT" 2>&1
