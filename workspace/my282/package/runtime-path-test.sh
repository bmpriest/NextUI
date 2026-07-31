#!/bin/sh

# Remove the temporary private SDL copies from the package search path so the
# recovery-wrapped smoke test exercises the A30 firmware runtime instead.

SYSTEM_LIB="/mnt/SDCARD/.system/my282/lib"
BACKUP_LIB="/tmp/my282-sdl-staged"

mkdir -p "$BACKUP_LIB"
for lib in \
	libSDL2-2.0.so.0 \
	libSDL2_image-2.0.so.0 \
	libSDL2_ttf-2.0.so.0
do
	if [ -f "$SYSTEM_LIB/$lib" ]; then
		mv "$SYSTEM_LIB/$lib" "$BACKUP_LIB/$lib"
	fi
done

TEST_SECONDS=15
export TEST_SECONDS
exec sh /tmp/cmd_to_run.sh
