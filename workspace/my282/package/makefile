PLATFORM ?= my282
.DEFAULT_GOAL := all

STAGE = build/stage
CARD = build/card
SYSTEM = $(STAGE)/.system/$(PLATFORM)
NEXTUI = ../../all/nextui/build/$(PLATFORM)/nextui.elf
MINARCH = ../../all/minarch/build/$(PLATFORM)/minarch.elf
KEYMON = ../keymon/keymon.elf
SETTINGS = ../../all/settings/build/$(PLATFORM)/settings.elf
GAMETIME = ../../all/gametime/build/$(PLATFORM)/gametime.elf
GAMETIMECTL = ../../all/gametimectl/build/$(PLATFORM)/gametimectl.elf
LIBGAMETIMEDB = ../../all/libgametimedb/build/$(PLATFORM)/libgametimedb.so
BATMON = ../../all/batmon/build/$(PLATFORM)/batmon.elf
BATTERY = ../../all/battery/build/$(PLATFORM)/battery.elf
LIBBATMONDB = ../../all/libbatmondb/build/$(PLATFORM)/libbatmondb.so
LIBMSTICK = ../libmstick/libmstick.so
CLOCK = ../../all/clock/build/$(PLATFORM)/clock.elf
MINPUT = ../../all/minput/build/$(PLATFORM)/minput.elf
MINUILIST = ../../all/minuilist/build/$(PLATFORM)/minui-list
NEXTCOMMANDER = ../other/NextCommander/output/NextCommander
NEXTCOMMANDER_RES = ../other/NextCommander/res
MSETTINGS = ../libmsettings/libmsettings.so
SSH_SERVER_ARCHIVE = ssh-server/SSH.Server.pak.zip
SSH_SERVER_SHA256 = e7e3a91369c682d142df0d139fb60badeea2bbb52bd6297defe118f1c6ca4c8b
SPRUCE_DROPBEAR = ssh/dropbearmulti
ZLIB = $(SYSROOT)/usr/lib/libz.so.1
SAMPLERATE = $(PREFIX_LOCAL)/lib/libsamplerate.so.0.2.2
LZ4 = $(PREFIX_LOCAL)/lib/liblz4.so.1
LIBZIP = $(PREFIX)/lib/libzip.so.5
CRYPTO = $(PREFIX)/lib/libcrypto.so.3
BZIP2 = $(PREFIX)/lib/libbz2.so.1.0
ATOMIC = $(SYSROOT)/lib/libatomic.so.1
LIBSTDCXX = $(shell $(CXX) -print-file-name=libstdc++.so.6)
LIBGCC = $(shell $(CC) -print-file-name=libgcc_s.so.1)
SQLITE = $(PREFIX_LOCAL)/lib/libsqlite3.so.0
SHADERS = ../../../skeleton/SYSTEM/tg5040/shaders
RES = ../../../skeleton/SYSTEM/res
SYSTEM_CFG = ../../../skeleton/SYSTEM/tg5040/system.cfg
PAK_SOURCE = ../../../skeleton/SYSTEM/tg5040/paks/Emus
TOOL_SOURCE = ../../../skeleton/EXTRAS/Tools/tg5040
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

all: $(NEXTUI) $(MINARCH) $(KEYMON) $(SETTINGS) $(GAMETIME) \
	$(GAMETIMECTL) $(LIBGAMETIMEDB) $(BATMON) $(BATTERY) \
	$(LIBBATMONDB) $(LIBMSTICK) $(CLOCK) $(MINPUT) $(MINUILIST) \
	$(NEXTCOMMANDER) $(MSETTINGS) $(CORE_FILES) $(SSH_SERVER_ARCHIVE) \
	$(SPRUCE_DROPBEAR)
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
	install -m 0755 "$(LIBGAMETIMEDB)" "$(SYSTEM)/lib/libgametimedb.so"
	install -m 0755 "$(LIBBATMONDB)" "$(SYSTEM)/lib/libbatmondb.so"
	install -m 0755 "$(LIBMSTICK)" "$(SYSTEM)/lib/libmstick.so"
	# The SQLite build records libsqlite3.so directly and the FAT card cannot
	# preserve its development/soname symlinks, so package both real names.
	install -m 0755 "$(SQLITE)" "$(SYSTEM)/lib/libsqlite3.so"
	install -m 0755 "$(SQLITE)" "$(SYSTEM)/lib/libsqlite3.so.0"
	install -m 0755 "$(GAMETIMECTL)" "$(SYSTEM)/bin/gametimectl.elf"
	install -m 0755 "$(BATMON)" "$(SYSTEM)/bin/batmon.elf"
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
		"$(CARD)/Tools/$(PLATFORM)/SSH Server.pak" \
		"$(CARD)/Tools/$(PLATFORM)/Battery.pak" \
		"$(CARD)/Tools/$(PLATFORM)/Game Tracker.pak" \
		"$(CARD)/Tools/$(PLATFORM)/Clock.pak" \
		"$(CARD)/Tools/$(PLATFORM)/Files.pak" \
		"$(CARD)/Tools/$(PLATFORM)/Input.pak"
	cp -R "$(SYSTEM)" "$(CARD)/.system/$(PLATFORM)"
	cp -R "$(RES)" "$(CARD)/.system/res"
	install -m 0755 "$(SETTINGS)" \
		"$(CARD)/Tools/$(PLATFORM)/Settings.pak/settings.elf"
	install -m 0755 settings-launch.sh \
		"$(CARD)/Tools/$(PLATFORM)/Settings.pak/launch.sh"
	install -m 0755 "$(BATTERY)" \
		"$(CARD)/Tools/$(PLATFORM)/Battery.pak/battery.elf"
	install -m 0755 battery/launch.sh \
		"$(CARD)/Tools/$(PLATFORM)/Battery.pak/launch.sh"
	install -m 0755 "$(GAMETIME)" \
		"$(CARD)/Tools/$(PLATFORM)/Game Tracker.pak/gametime.elf"
	install -m 0755 "$(TOOL_SOURCE)/Game Tracker.pak/launch.sh" \
		"$(CARD)/Tools/$(PLATFORM)/Game Tracker.pak/launch.sh"
	install -m 0755 "$(CLOCK)" \
		"$(CARD)/Tools/$(PLATFORM)/Clock.pak/clock.elf"
	install -m 0755 "$(TOOL_SOURCE)/Clock.pak/launch.sh" \
		"$(CARD)/Tools/$(PLATFORM)/Clock.pak/launch.sh"
	install -m 0755 "$(MINPUT)" \
		"$(CARD)/Tools/$(PLATFORM)/Input.pak/minput.elf"
	install -m 0755 "$(TOOL_SOURCE)/Input.pak/launch.sh" \
		"$(CARD)/Tools/$(PLATFORM)/Input.pak/launch.sh"
	install -m 0755 "$(NEXTCOMMANDER)" \
		"$(CARD)/Tools/$(PLATFORM)/Files.pak/NextCommander"
	cp -R "$(NEXTCOMMANDER_RES)" \
		"$(CARD)/Tools/$(PLATFORM)/Files.pak/res"
	install -m 0755 files/launch.sh \
		"$(CARD)/Tools/$(PLATFORM)/Files.pak/launch.sh"
	install -m 0644 files/my282.cfg \
		"$(CARD)/Tools/$(PLATFORM)/Files.pak/my282.cfg"
	printf '%s  %s\n' "$(SSH_SERVER_SHA256)" "$(SSH_SERVER_ARCHIVE)" | \
		sha256sum -c -
	unzip -q "$(SSH_SERVER_ARCHIVE)" -d \
		"$(CARD)/Tools/$(PLATFORM)/SSH Server.pak"
	install -m 0755 "$(MINUILIST)" \
		"$(CARD)/Tools/$(PLATFORM)/SSH Server.pak/bin/$(PLATFORM)/minui-list"
	install -m 0755 "$(SPRUCE_DROPBEAR)" \
		"$(CARD)/Tools/$(PLATFORM)/SSH Server.pak/bin/arm/dropbearmulti"
	install -m 0755 ssh-server/launch.sh \
		ssh-server/service-on ssh-server/service-off \
		ssh-server/service-is-running \
		"$(CARD)/Tools/$(PLATFORM)/SSH Server.pak/bin"
	mv "$(CARD)/Tools/$(PLATFORM)/SSH Server.pak/bin/launch.sh" \
		"$(CARD)/Tools/$(PLATFORM)/SSH Server.pak/launch.sh"
	install -m 0755 boot-dispatcher.sh "$(CARD)/.tmp_update/updater"
	install -m 0755 boot.sh "$(CARD)/.tmp_update/$(PLATFORM).sh"
	install -m 0755 stock-mainui.sh "$(CARD)/miyoo/app/MainUI"
	install -m 0755 stock-keymon.sh "$(CARD)/miyoo/app/keymon"
	install -m 0644 NEXTUI-MY282.txt "$(CARD)/NEXTUI-MY282.txt"

clean:
	rm -rf build
