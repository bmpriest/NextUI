ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

PLATFORM ?= $(UNION_PLATFORM)
TARGET = keymon.elf
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Os -Wl,--gc-sections -s
CFLAGS += -I$(PREFIX_LOCAL)/include -L$(PREFIX_LOCAL)/lib
LDLIBS = -lmsettings -lrt -ldl

.PHONY: all clean

all: $(TARGET)

$(TARGET): keymon.c $(PREFIX_LOCAL)/include/msettings.h
	$(CC) $(CFLAGS) keymon.c -o $@ $(LDLIBS)

$(PREFIX_LOCAL)/include/msettings.h:
	$(MAKE) -C ../libmsettings

clean:
	rm -f $(TARGET)
