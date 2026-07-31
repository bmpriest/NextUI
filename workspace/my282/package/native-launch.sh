#!/bin/sh

EMU_TAG=$(basename "$(dirname "$0")" .pak)

case "$EMU_TAG" in
	FC) EMU_EXE=fceumm ;;
	GB|GBC) EMU_EXE=gambatte ;;
	GBA) EMU_EXE=gpsp ;;
	MD) EMU_EXE=picodrive ;;
	PS) EMU_EXE=pcsx_rearmed ;;
	SFC) EMU_EXE=snes9x2005 ;;
	*)
		echo "Unsupported native my282 emulator: $EMU_TAG" >&2
		exit 1
		;;
esac

ROM="$1"
mkdir -p "$BIOS_PATH/$EMU_TAG"
mkdir -p "$SAVES_PATH/$EMU_TAG"
mkdir -p "$CHEATS_PATH/$EMU_TAG"
mkdir -p "$LOGS_PATH"
export HOME="$USERDATA_PATH"
cd "$HOME" || exit 1

LOG_FILE="$LOGS_PATH/$EMU_TAG.txt"
if ( : > "$LOG_FILE" ) 2>/dev/null; then
	exec > "$LOG_FILE" 2>&1
else
	exec > "/tmp/nextui-$EMU_TAG.txt" 2>&1
fi

echo "launcher: system=$EMU_TAG core=$EMU_EXE rom=$ROM"
exec minarch.elf "$CORES_PATH/${EMU_EXE}_libretro.so" "$ROM"
