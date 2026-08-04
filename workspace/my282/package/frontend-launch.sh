#!/bin/sh

export PLATFORM="my282"
export SDCARD_PATH="/mnt/SDCARD"
export BIOS_PATH="$SDCARD_PATH/Bios"
export ROMS_PATH="$SDCARD_PATH/Roms"
export SAVES_PATH="$SDCARD_PATH/Saves"
export CHEATS_PATH="$SDCARD_PATH/Cheats"
export SYSTEM_PATH="$SDCARD_PATH/.system/$PLATFORM"
export CORES_PATH="$SYSTEM_PATH/cores"
export USERDATA_PATH="$SDCARD_PATH/.userdata/$PLATFORM"
export SHARED_USERDATA_PATH="$SDCARD_PATH/.userdata/shared"
export LOGS_PATH="$USERDATA_PATH/logs"
export HOME="$USERDATA_PATH"
export IS_NEXT="yes"
export MODEL=282
export DEVICE=my282

mkdir -p "$BIOS_PATH" "$ROMS_PATH" "$SAVES_PATH" "$CHEATS_PATH"
mkdir -p "$USERDATA_PATH" "$LOGS_PATH" "$SHARED_USERDATA_PATH/.minui"

# The A30 stock boot does not restore NextUI's Wi-Fi state. Reconnect here
# when Wi-Fi was enabled in the persisted NextUI settings.
if grep -q '^wifi=1$' "$SHARED_USERDATA_PATH/minuisettings.txt" 2>/dev/null; then
	"$SYSTEM_PATH/etc/wifi/wifi_init.sh" start
fi

export SDL_VIDEODRIVER="mali"
export SDL_AUDIODRIVER="alsa"
export LD_LIBRARY_PATH="$SYSTEM_PATH/lib:/usr/miyoo/lib:/usr/lib:/lib"
export PATH="$SYSTEM_PATH/bin:/usr/miyoo/bin:/usr/miyoo/sbin:/usr/bin:/usr/sbin:/bin:/sbin"

# Keep the kernel button order identical to the mapping validated under
# Spruce. NextUI reads event0/event3 and the physical UART itself.
BUTTON_CONFIG="/sys/module/gpio_keys_polled/parameters/button_config"
if [ -w "$BUTTON_CONFIG" ]; then
	echo L,L2,R,R2,X,A,B,Y > "$BUTTON_CONFIG"
fi
killall joypad joystickinput 2>/dev/null

KEYMON_PID=""
BATMON_PID=""
run_logged() {
	LOG_FILE="$1"
	TMP_LOG="$2"
	shift 2
	if ( : > "$LOG_FILE" ) 2>/dev/null; then
		"$@" > "$LOG_FILE" 2>&1
	else
		"$@" > "$TMP_LOG" 2>&1
	fi
}

cleanup_daemons() {
	if [ -n "$BATMON_PID" ]; then
		kill -TERM "$BATMON_PID" 2>/dev/null
		wait "$BATMON_PID" 2>/dev/null
	fi
	if [ -n "$KEYMON_PID" ]; then
		kill -TERM "$KEYMON_PID" 2>/dev/null
		wait "$KEYMON_PID" 2>/dev/null
	fi
}
trap cleanup_daemons EXIT
trap 'exit 0' HUP INT TERM

if ( : > "$LOGS_PATH/keymon.txt" ) 2>/dev/null; then
	"$SYSTEM_PATH/bin/keymon.elf" > "$LOGS_PATH/keymon.txt" 2>&1 &
else
	"$SYSTEM_PATH/bin/keymon.elf" > /tmp/nextui-keymon.txt 2>&1 &
fi
KEYMON_PID=$!

if ( : > "$LOGS_PATH/batmon.txt" ) 2>/dev/null; then
	"$SYSTEM_PATH/bin/batmon.elf" > "$LOGS_PATH/batmon.txt" 2>&1 &
else
	"$SYSTEM_PATH/bin/batmon.elf" > /tmp/nextui-batmon.txt 2>&1 &
fi
BATMON_PID=$!

# Support composable service hooks used by maintained MinUI/NextUI paks such
# as SSH Server's optional start-on-boot setting.
AUTO_PATH="$USERDATA_PATH/auto.sh"
if [ -f "$AUTO_PATH" ]; then
	"$AUTO_PATH"
fi

EXEC_PATH="/tmp/nextui_exec"
NEXT_PATH="/tmp/next"
rm -f "$NEXT_PATH"
touch "$EXEC_PATH"
sync

while [ -f "$EXEC_PATH" ]; do
	run_logged "$LOGS_PATH/nextui.txt" /tmp/nextui.txt \
		"$SYSTEM_PATH/bin/nextui.elf"

	if [ -f "$NEXT_PATH" ]; then
		CMD="$(cat "$NEXT_PATH")"
		eval "$CMD"
		rm -f "$NEXT_PATH"
	fi

	if [ -f /tmp/poweroff ]; then
		exit 0
	fi
	if [ -f /tmp/reboot ]; then
		exit 0
	fi
done

sync
exit 0
