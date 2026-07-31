#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "api.h"
#include "utils.h"

PerfProfile perf;
PAD_Context pad;

void PAD_setAnalog(int neg_id, int pos_id, int value, int repeat_at) {
	(void)neg_id;
	(void)pos_id;
	(void)value;
	(void)repeat_at;
}

void LOG_note(int level, const char *format, ...) {
	(void)level;
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int getInt(char *path) {
	FILE *file = fopen(path, "r");
	if (!file) return 0;
	int value = 0;
	fscanf(file, "%d", &value);
	fclose(file);
	return value;
}

void getFile(char *path, char *buffer, size_t buffer_size) {
	if (buffer_size == 0) return;
	buffer[0] = '\0';
	FILE *file = fopen(path, "r");
	if (!file) return;
	if (fgets(buffer, buffer_size, file))
		buffer[strcspn(buffer, "\r\n")] = '\0';
	fclose(file);
}

void putInt(char *path, int value) {
	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "putInt open %s: %s\n", path, strerror(errno));
		return;
	}
	char text[32];
	int length = snprintf(text, sizeof(text), "%d", value);
	if (write(fd, text, length) != length)
		fprintf(stderr, "putInt write %s: %s\n", path, strerror(errno));
	close(fd);
}

void touch(char *path) {
	int fd = open(path, O_CREAT | O_WRONLY, 0644);
	if (fd >= 0) close(fd);
}

static void print_buttons(int mask, const char *action) {
	static const struct {
		int button;
		const char *name;
	} buttons[] = {
		{BTN_DPAD_UP, "UP"}, {BTN_DPAD_DOWN, "DOWN"},
		{BTN_DPAD_LEFT, "LEFT"}, {BTN_DPAD_RIGHT, "RIGHT"},
		{BTN_A, "A"}, {BTN_B, "B"}, {BTN_X, "X"}, {BTN_Y, "Y"},
		{BTN_START, "START"}, {BTN_SELECT, "SELECT"},
		{BTN_MENU, "MENU"}, {BTN_L1, "L1"}, {BTN_L2, "L2"},
		{BTN_R1, "R1"}, {BTN_R2, "R2"}, {BTN_PLUS, "VOL+"},
		{BTN_MINUS, "VOL-"}, {BTN_POWER, "POWER"},
	};
	for (size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
		if (mask & buttons[i].button)
			printf("%s %s\n", action, buttons[i].name);
	}
}

static int input_test(void) {
	if (SDL_Init(SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "SDL timer init failed: %s\n", SDL_GetError());
		return 1;
	}
	PLAT_initInput();
	printf("input capture: 30 seconds\n");
	fflush(stdout);

	int previous_x = 0;
	int previous_y = 0;
	uint32_t end = SDL_GetTicks() + 30000;
	while ((Sint32)(end - SDL_GetTicks()) > 0) {
		PLAT_pollInput();
		print_buttons(pad.just_pressed, "press");
		print_buttons(pad.just_released, "release");
		if (abs(pad.laxis.x - previous_x) >= 1024 ||
		    abs(pad.laxis.y - previous_y) >= 1024) {
			printf("stick %d %d\n", pad.laxis.x, pad.laxis.y);
			previous_x = pad.laxis.x;
			previous_y = pad.laxis.y;
		}
		fflush(stdout);
		SDL_Delay(5);
	}

	PLAT_quitInput();
	SDL_Quit();
	return 0;
}

int main(int argc, char **argv) {
	if (argc > 1 && strcmp(argv[1], "--input") == 0)
		return input_test();

	int charging = 0;
	int charge = 0;
	char firmware[128] = {0};

	PLAT_getBatteryStatusFine(&charging, &charge);
	PLAT_getCPUTemp();
	PLAT_getCPUSpeed();
	PLAT_getOsVersionInfo(firmware, sizeof(firmware));

	printf("model: %s\n", PLAT_getModel());
	printf("firmware: %s\n", firmware);
	printf("battery: %d%% charging=%d\n", charge, charging);
	printf("CPU: %d MHz %d C\n", perf.cpu_speed, perf.cpu_temp);
	printf("USB data connected: %d\n", PLAT_isUSBConnected());

	PLAT_setCPUSpeed(CPU_SPEED_POWERSAVE);
	char governor[32] = {0};
	getFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",
	        governor, sizeof(governor));
	int minimum = getInt(
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
	int maximum = getInt(
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
	printf("CPU powersave: %s %d-%d kHz\n", governor, minimum, maximum);
	int powersave_ok = strcmp(governor, "conservative") == 0 &&
		minimum == 240000 && maximum == 1008000;

	PLAT_setCPUSpeed(CPU_SPEED_AUTO);
	getFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",
	        governor, sizeof(governor));
	minimum = getInt(
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
	maximum = getInt(
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
	printf("CPU restored: %s %d-%d kHz\n", governor, minimum, maximum);
	int restored = strcmp(governor, "conservative") == 0 &&
		minimum == 648000 && maximum == 1344000;

	return powersave_ok && restored ? 0 : 1;
}
