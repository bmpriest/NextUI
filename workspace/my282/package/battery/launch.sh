#!/bin/sh

DIR="$(dirname "$0")"
cd "$DIR" || exit 1

exec ./battery.elf
