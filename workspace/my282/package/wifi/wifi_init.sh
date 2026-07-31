#!/bin/sh

WIFI_INTERFACE="wlan0"
WIFI_SOCKET_DIR="/tmp/nextui-wifi"
WIFI_STATE_DIR="${USERDATA_PATH:-/mnt/SDCARD/.userdata/my282}/wifi"
WPA_CONF="$WIFI_STATE_DIR/wpa_supplicant.conf"
UDHCPC_SCRIPT="${SYSTEM_PATH:-/mnt/SDCARD/.system/my282}/etc/wifi/udhcpc.script"

start_wifi() {
	mkdir -p "$WIFI_STATE_DIR" "$WIFI_SOCKET_DIR"
	if [ ! -f "$WPA_CONF" ]; then
		printf '%s\n' \
			"ctrl_interface=$WIFI_SOCKET_DIR" \
			"update_config=1" > "$WPA_CONF"
	fi

	if [ ! -d "/sys/class/net/$WIFI_INTERFACE" ]; then
		modprobe 8188fu 2>/dev/null || modprobe rtl8188fu 2>/dev/null
		sleep 1
	fi

	ifconfig lo up 2>/dev/null
	ifconfig "$WIFI_INTERFACE" up 2>/dev/null || return 1

	# Take ownership if stock left a supplicant using a different control path.
	if [ ! -S "$WIFI_SOCKET_DIR/$WIFI_INTERFACE" ]; then
		killall wpa_supplicant 2>/dev/null
		wpa_supplicant -B -D nl80211 -i "$WIFI_INTERFACE" \
			-c "$WPA_CONF" >/dev/null 2>&1 || return 1
		sleep 1
	fi

	if ! pidof udhcpc >/dev/null 2>&1; then
		udhcpc -i "$WIFI_INTERFACE" -s "$UDHCPC_SCRIPT" \
			-b >/dev/null 2>&1
	fi
}

stop_wifi() {
	wpa_cli -p "$WIFI_SOCKET_DIR" -i "$WIFI_INTERFACE" \
		disconnect >/dev/null 2>&1
	killall wpa_supplicant 2>/dev/null
	killall udhcpc 2>/dev/null
	ifconfig "$WIFI_INTERFACE" down 2>/dev/null
	rm -rf "$WIFI_SOCKET_DIR"
}

case "$1" in
	start|"") start_wifi ;;
	stop) stop_wifi ;;
	*) echo "Usage: $0 {start|stop}"; exit 1 ;;
esac
