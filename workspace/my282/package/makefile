PLATFORM ?= my282
.DEFAULT_GOAL := all

STAGE = build/stage
CARD = build/card
SYSTEM = $(STAGE)/.system/$(PLATFORM)
NEXTUI = ../../all/nextui/build/$(PLATFORM)/nextui.elf
MINARCH = ../../all/minarch/build/$(PLATFORM)/minarch.elf
KEYMON = ../keymon/keymon.elf
SETTINGS = ../../all/settings/build/$(PLATFORM)/settings.elf
MSETTINGS = ../libmsettings/libmsettings.so
DROPBEAR = ssh/dropbearmulti
ZLIB = $(SYSROOT)/usr/lib/libz.so.1
SAMPLERATE = $(PREFIX_LOCAL)/lib/libsamplerate.so.0.2.2
LZ4 = $(PREFIX_LOCAL)/lib/liblz4.so.1
LIBZIP = $(PREFIX)/lib/libzip.so.5
CRYPTO = $(PREFIX)/lib/libcrypto.so.3
BZIP2 = $(PREFIX)/lib/libbz2.so.1.0
ATOMIC = $(SYSROOT)/lib/libatomic.so.1
LIBSTDCXX = $(shell $(CXX) -print-file-name=libstdc++.so.6)
LIBGCC = $(shell $(CC) -print-file-name=libgcc_s.so.1)
SHADERS = ../../../skeleton/SYSTEM/tg5040/shaders
RES = ../../../skeleton/SYSTEM/res
SYSTEM_CFG = ../../../skeleton/SYSTEM/tg5040/system.cfg
PAK_SOURCE = ../../../skeleton/SYSTEM/tg5040/paks/Emus
CORE_OUTPUT = ../cores/output
CORE_FILES = $(CORE_OUTPUT)/fceumm_libretro.so
CORE_FILES += $(CORE_OUTPUT)/gambatte_libretro.so
CORE_FILES += $(CORE_OUTPUT)/gpsp_libretro.so
CORE_FILES += $(CORE_OUTPUT)/pcsx_rearmed_libretro.so
CORE_FILES += $(CORE_OUTPUT)/picodrive_libretro.so
CORE_FILES += $(CORE_OUTPUT)/snes9x2005_libretro.so
NATIVE_EMUS = FC GB GBA GBC MD PS SFC

.PHONY: all clean

$(CORE_FILES):
	$(MAKE) -C ../cores

all: $(NEXTUI) $(MINARCH) $(KEYMON) $(SETTINGS) $(MSETTINGS) $(CORE_FILES) $(DROPBEAR)
	rm -rf "$(SYSTEM)/cores" "$(SYSTEM)/lib" "$(SYSTEM)/paks/Emus" \
		"$(SYSTEM)/paks/MinUI.pak"
	mkdir -p "$(SYSTEM)/bin" "$(SYSTEM)/cores" "$(SYSTEM)/lib" \
		"$(SYSTEM)/paks/MinUI.pak" "$(SYSTEM)/shaders" \
		"$(SYSTEM)/etc/wifi"
	install -m 0755 "$(NEXTUI)" "$(SYSTEM)/bin/nextui.elf"
	install -m 0755 "$(MINARCH)" "$(SYSTEM)/bin/minarch.elf"
	install -m 0755 "$(KEYMON)" "$(SYSTEM)/bin/keymon.elf"
	install -m 0755 safe-poweroff.sh "$(SYSTEM)/bin/safe-poweroff.sh"
	install -m 0755 $(CORE_FILES) "$(SYSTEM)/cores"
	install -m 0755 "$(MSETTINGS)" "$(SYSTEM)/lib/libmsettings.so"
	# SDL2 is part of the A30 firmware ABI. The SDK copy is a generic X11
	# build and cannot open the Mali framebuffer, so do not shadow the
	# firmware's /usr/miyoo/lib implementation here.
	install -m 0755 "$(ZLIB)" "$(SYSTEM)/lib/libz.so.1"
	install -m 0755 "$(SAMPLERATE)" \
		"$(SYSTEM)/lib/libsamplerate.so.0.2.2"
	# The A30 SD card is FAT and cannot preserve ELF soname symlinks.
	install -m 0755 "$(SAMPLERATE)" \
		"$(SYSTEM)/lib/libsamplerate.so.0"
	install -m 0755 "$(LZ4)" "$(SYSTEM)/lib/liblz4.so.1"
	install -m 0755 "$(LIBZIP)" "$(SYSTEM)/lib/libzip.so.5"
	install -m 0755 "$(CRYPTO)" "$(SYSTEM)/lib/libcrypto.so.3"
	install -m 0755 "$(BZIP2)" "$(SYSTEM)/lib/libbz2.so.1.0"
	install -m 0755 "$(ATOMIC)" "$(SYSTEM)/lib/libatomic.so.1"
	install -m 0755 "$(LIBSTDCXX)" "$(SYSTEM)/lib/libstdc++.so.6"
	install -m 0755 "$(LIBGCC)" "$(SYSTEM)/lib/libgcc_s.so.1"
	install -m 0644 "$(SHADERS)/default.glsl" \
		"$(SYSTEM)/shaders/default.glsl"
	install -m 0644 "$(SHADERS)/overlay.glsl" \
		"$(SYSTEM)/shaders/overlay.glsl"
	install -m 0644 "$(SHADERS)/noshader.glsl" \
		"$(SYSTEM)/shaders/noshader.glsl"
	install -m 0644 "$(SYSTEM_CFG)" "$(SYSTEM)/system.cfg"
	install -m 0755 frontend-launch.sh \
		"$(SYSTEM)/paks/MinUI.pak/launch.sh"
	install -m 0755 wifi/wifi_init.sh wifi/udhcpc.script \
		"$(SYSTEM)/etc/wifi"
	for emu in $(NATIVE_EMUS); do \
		mkdir -p "$(SYSTEM)/paks/Emus/$$emu.pak"; \
		install -m 0755 native-launch.sh \
			"$(SYSTEM)/paks/Emus/$$emu.pak/launch.sh"; \
		install -m 0644 "$(PAK_SOURCE)/$$emu.pak/default.cfg" \
			"$(SYSTEM)/paks/Emus/$$emu.pak/default.cfg"; \
	done
	# Default PS1 to the standard controller so the A30 stick mirrors the
	# D-pad. DualShock remains selectable per game for analog-aware titles.
	sed -i 's/^minarch_gamepad_type = 1$$/minarch_gamepad_type = 0/' \
		"$(SYSTEM)/paks/Emus/PS.pak/default.cfg"
	rm -rf "$(CARD)"
	mkdir -p "$(CARD)/.system" "$(CARD)/.tmp_update" \
		"$(CARD)/miyoo/app" "$(CARD)/Bios" "$(CARD)/Roms" \
		"$(CARD)/Saves" "$(CARD)/Tools/$(PLATFORM)/Settings.pak" \
		"$(CARD)/Tools/$(PLATFORM)/SSH.pak"
	cp -R "$(SYSTEM)" "$(CARD)/.system/$(PLATFORM)"
	cp -R "$(RES)" "$(CARD)/.system/res"
	install -m 0755 "$(SETTINGS)" \
		"$(CARD)/Tools/$(PLATFORM)/Settings.pak/settings.elf"
	install -m 0755 settings-launch.sh \
		"$(CARD)/Tools/$(PLATFORM)/Settings.pak/launch.sh"
	install -m 0755 ssh/launch.sh ssh/shell-wrapper.sh "$(DROPBEAR)" \
		"$(CARD)/Tools/$(PLATFORM)/SSH.pak"
	install -m 0644 ssh/passwd ssh/README.txt \
		"$(CARD)/Tools/$(PLATFORM)/SSH.pak"
	install -m 0755 boot-dispatcher.sh "$(CARD)/.tmp_update/updater"
	install -m 0755 boot.sh "$(CARD)/.tmp_update/$(PLATFORM).sh"
	install -m 0755 stock-mainui.sh "$(CARD)/miyoo/app/MainUI"
	install -m 0755 stock-keymon.sh "$(CARD)/miyoo/app/keymon"
	install -m 0644 NEXTUI-MY282.txt "$(CARD)/NEXTUI-MY282.txt"

clean:
	rm -rf build
