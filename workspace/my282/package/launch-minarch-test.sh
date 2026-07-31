#!/bin/sh

# Temporary Spruce-side MinArch smoke test. Arguments are a 32-bit libretro
# core and a ROM already present on the device. Spruce is always restored.

if [ "$#" -ne 2 ]; then
	echo "usage: $0 CORE_PATH ROM_PATH" >&2
	exit 2
fi

CORE_PATH="$1"
ROM_PATH="$2"
PLATFORM="my282"
SDCARD_PATH="/mnt/SDCARD"
SYSTEM_PATH="$SDCARD_PATH/.system/$PLATFORM"
USERDATA_PATH="$SDCARD_PATH/.userdata/$PLATFORM"
SHARED_USERDATA_PATH="$SDCARD_PATH/.userdata/shared"
LOGS_PATH="$USERDATA_PATH/logs"
MAINUI_PID="$(pidof MainUI 2>/dev/null)"
MINARCH_PID=""
JOYPAD_WAS_RUNNING=0
JOYSTICKINPUT_PID="$(pidof joystickinput 2>/dev/null)"

if [ ! -f "$CORE_PATH" ]; then
	echo "core not found: $CORE_PATH" >&2
	exit 1
fi
if [ ! -f "$ROM_PATH" ]; then
	echo "ROM not found: $ROM_PATH" >&2
	exit 1
fi

restore_spruce() {
	trap - EXIT HUP INT TERM
	if [ -n "$MINARCH_PID" ]; then
		kill -TERM "$MINARCH_PID" 2>/dev/null
		remaining=4
		while kill -0 "$MINARCH_PID" 2>/dev/null &&
		      [ "$remaining" -gt 0 ]; do
			sleep 1
			remaining=$((remaining - 1))
		done
		if kill -0 "$MINARCH_PID" 2>/dev/null; then
			kill -KILL "$MINARCH_PID" 2>/dev/null
		fi
		wait "$MINARCH_PID" 2>/dev/null
	fi
	if [ "$JOYPAD_WAS_RUNNING" -eq 1 ] &&
	   ! pidof joypad >/dev/null 2>&1; then
		(
			cd /mnt/SDCARD/spruce/bin || exit
			./joypad /dev/input/event3
		) &
	fi
	if [ -n "$JOYSTICKINPUT_PID" ]; then
		kill -CONT "$JOYSTICKINPUT_PID" 2>/dev/null
		kill -USR2 "$JOYSTICKINPUT_PID" 2>/dev/null
	fi
	if [ -n "$MAINUI_PID" ]; then
		kill -CONT "$MAINUI_PID" 2>/dev/null
	fi
}
trap restore_spruce EXIT
trap 'exit 0' HUP INT TERM

mkdir -p "$USERDATA_PATH" "$SHARED_USERDATA_PATH/.minui" "$LOGS_PATH"

export PLATFORM SDCARD_PATH SYSTEM_PATH USERDATA_PATH SHARED_USERDATA_PATH
export LOGS_PATH
export BIOS_PATH="$SDCARD_PATH/Bios"
export ROMS_PATH="$SDCARD_PATH/Roms"
export SAVES_PATH="$SDCARD_PATH/Saves"
export CHEATS_PATH="$SDCARD_PATH/Cheats"
export HOME="$USERDATA_PATH"
export SDL_VIDEODRIVER="mali"
export SDL_AUDIODRIVER="alsa"
export LD_LIBRARY_PATH="$SYSTEM_PATH/lib:$SDCARD_PATH/spruce/a30/lib:$SDCARD_PATH/miyoo/lib:/usr/miyoo/lib:/usr/lib:/lib"
export PATH="$SYSTEM_PATH/bin:/usr/miyoo/bin:/usr/miyoo/sbin:$PATH"

if [ -n "$MAINUI_PID" ]; then
	kill -STOP "$MAINUI_PID"
fi
if pidof joypad >/dev/null 2>&1; then
	JOYPAD_WAS_RUNNING=1
	killall -TERM joypad 2>/dev/null
	sleep 1
	if pidof joypad >/dev/null 2>&1; then
		killall -KILL joypad 2>/dev/null
	fi
fi
if [ -n "$JOYSTICKINPUT_PID" ]; then
	kill -STOP "$JOYSTICKINPUT_PID" 2>/dev/null
fi

cd "$HOME" || exit 1
"$SYSTEM_PATH/bin/minarch.elf" "$CORE_PATH" "$ROM_PATH" \
	> "$LOGS_PATH/minarch-test.txt" 2>&1 &
MINARCH_PID="$!"

seconds="${TEST_SECONDS:-60}"
case "$seconds" in
	''|*[!0-9]*) seconds=60 ;;
esac
while [ "$seconds" -gt 0 ] && kill -0 "$MINARCH_PID" 2>/dev/null; do
	sleep 1
	seconds=$((seconds - 1))
done
