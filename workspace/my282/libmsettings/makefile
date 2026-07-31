include ../platform/makefile.env

TARGET = msettings
CC = $(CROSS_COMPILE)gcc
CFLAGS := $(ARCH) -O2 -Wall -Wextra -Werror -fPIC
LDFLAGS := -shared -lrt -ldl -lm -lasound

.PHONY: all build clean

all: build

build: lib$(TARGET).so
	mkdir -p "$(PREFIX_LOCAL)/include" "$(PREFIX_LOCAL)/lib"
	cp "$(TARGET).h" "$(PREFIX_LOCAL)/include/"
	cp "lib$(TARGET).so" "$(PREFIX_LOCAL)/lib/"

lib$(TARGET).so: $(TARGET).c $(TARGET).h
	$(CC) $(CFLAGS) -o "$@" "$(TARGET).c" $(LDFLAGS)

clean:
	rm -f "lib$(TARGET).so"
	rm -f "$(PREFIX_LOCAL)/include/$(TARGET).h"
	rm -f "$(PREFIX_LOCAL)/lib/lib$(TARGET).so"
