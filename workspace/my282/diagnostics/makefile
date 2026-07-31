include ../platform/makefile.env

PROBE = build/my282-probe.elf
SETTINGS_SMOKE = build/my282-settings-smoke.elf
PLATFORM_SMOKE = build/my282-platform-smoke.elf
CC = $(CROSS_COMPILE)gcc

CFLAGS += -std=gnu99 -Wall -Wextra
PROBE_LDFLAGS = -L$(PREFIX_LOCAL)/lib -L$(PREFIX)/lib \
	$$(pkg-config --libs sdl2) -lGLESv2 -lEGL -ldl -lm

.PHONY: all clean

all: $(PROBE) $(SETTINGS_SMOKE) $(PLATFORM_SMOKE) platform-check

$(PROBE): probe.c
	mkdir -p build
	$(CC) probe.c -o $@ $(CFLAGS) $(PROBE_LDFLAGS)

$(SETTINGS_SMOKE): settings-smoke.c ../libmsettings/libmsettings.so
	mkdir -p build
	$(CC) settings-smoke.c -o $@ $(ARCH) -O2 -Wall -Wextra -Werror \
		-I../libmsettings -L../libmsettings -lmsettings \
		-Wl,-rpath,'$$ORIGIN'

$(PLATFORM_SMOKE): platform-smoke.c ../platform/platform.c
	mkdir -p build
	$(CC) platform-smoke.c ../platform/platform.c -o $@ $(CFLAGS) \
		-I../platform -I../libmsettings -I../../all/common \
		-DMY282_PLATFORM_CORE_ONLY -ffunction-sections -fdata-sections \
		-Wl,--gc-sections $$(pkg-config --libs sdl2)

../libmsettings/libmsettings.so:
	$(MAKE) -C ../libmsettings

.PHONY: platform-check
platform-check: ../platform/platform.c ../platform/platform.h
	$(CC) -fsyntax-only ../platform/platform.c $(CFLAGS) \
		-I../platform -I../libmsettings -I../../all/common \
		-DMY282_PLATFORM_CORE_ONLY

clean:
	rm -rf build
