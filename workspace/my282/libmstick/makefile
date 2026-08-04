include ../platform/makefile.env

TARGET = mstick
PRODUCT = lib$(TARGET).so
CC = $(CROSS_COMPILE)gcc

CFLAGS += $(ARCH) -fPIC -O3 -Wall -Werror
MSTICK_LDFLAGS = -shared -ldl -lpthread -lrt -s

.PHONY: all clean

all:
	$(CC) -c $(TARGET).c -o $(TARGET).o $(CFLAGS)
	$(CC) $(TARGET).o -o $(PRODUCT) $(MSTICK_LDFLAGS)

clean:
	rm -f $(TARGET).o $(PRODUCT)
