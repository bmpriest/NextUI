#!/bin/sh

cd /mnt/SDCARD || exit 1

if [ -n "$SSH_ORIGINAL_COMMAND" ]; then
	exec /bin/sh -c "$SSH_ORIGINAL_COMMAND"
fi

exec /bin/sh
