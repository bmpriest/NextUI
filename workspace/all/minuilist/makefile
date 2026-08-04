ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
	$(error please specify PLATFORM)
endif

include ../../$(PLATFORM)/platform/makefile.env

TARGET = minui-list
INCDIR = -I. -I../common/ -I../../$(PLATFORM)/platform/
SOURCE = minui-list.c parson/parson.c ../common/scaler.c ../common/utils.c \
	../common/api.c ../common/config.c ../../$(PLATFORM)/platform/platform.c

CC = $(CROSS_COMPILE)gcc
CFLAGS += $(OPT) $(INCDIR) -DPLATFORM=\"$(PLATFORM)\" \
	-DPLATFORM_NEXTUI -std=gnu99
LDFLAGS += -lmsettings

PRODUCT = build/$(PLATFORM)/$(TARGET)

.PHONY: all clean

all: $(PREFIX_LOCAL)/include/msettings.h
	mkdir -p build/$(PLATFORM)
	$(CC) $(SOURCE) -o $(PRODUCT) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PRODUCT)

$(PREFIX_LOCAL)/include/msettings.h:
	cd ../../$(PLATFORM)/libmsettings && make
