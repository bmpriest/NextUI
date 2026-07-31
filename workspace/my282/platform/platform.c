// Miyoo A30 / my282

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <linux/fb.h>
#include <msettings.h>

#include "defines.h"
#include "platform.h"
#include "api.h"
#include "utils.h"

#define BATTERY_PATH "/sys/class/power_supply/battery/capacity"
#define AC_PATH "/sys/class/power_supply/ac/online"
#define CPUFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq"
#define CPU_TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define RUMBLE_PATH "/sys/devices/virtual/timed_output/vibrator/enable"

// Keep linux/input.h out of this translation unit because its BTN_* names
// collide with NextUI's logical button names.
struct my282_input_event {
	struct timeval time;
	unsigned short type;
	unsigned short code;
	int value;
};

#define MY282_EV_KEY 0x01
#define MY282_INPUT_COUNT 2

static int input_fds[MY282_INPUT_COUNT] = {-1, -1};
static int stick_fd = -1;
static struct termios stick_original_termios;
static int stick_has_original_termios;
static unsigned char stick_buffer[64];
static size_t stick_buffer_size;
static int stick_x;
static int stick_y;

static int clamp_percent(int value) {
	if (value < 0) return 0;
	if (value > 100) return 100;
	return value;
}

static int scale_stick_axis(unsigned char raw) {
	int delta = (int)raw - 128;
	if (delta > -10 && delta < 10) return 0;
	if (delta <= -64) return -32768;
	if (delta >= 64) return 32767;
	return delta * 512;
}

static void parse_stick_bytes(const unsigned char *bytes, size_t count) {
	if (count > sizeof(stick_buffer) - stick_buffer_size) {
		// Retain only a possible partial frame; the stream is continuous and
		// the next 0xff marker will resynchronize it.
		stick_buffer_size = 0;
	}
	if (count > sizeof(stick_buffer)) {
		bytes += count - sizeof(stick_buffer);
		count = sizeof(stick_buffer);
	}
	memcpy(stick_buffer + stick_buffer_size, bytes, count);
	stick_buffer_size += count;

	while (stick_buffer_size >= 6) {
		size_t start = 0;
		while (start < stick_buffer_size &&
		       stick_buffer[start] != 0xff)
			++start;
		if (start > 0) {
			memmove(stick_buffer, stick_buffer + start,
			        stick_buffer_size - start);
			stick_buffer_size -= start;
		}
		if (stick_buffer_size < 6) break;

		if (stick_buffer[5] == 0xfe) {
			stick_x = scale_stick_axis(stick_buffer[3]);
			stick_y = scale_stick_axis(stick_buffer[4]);
			memmove(stick_buffer, stick_buffer + 6,
			        stick_buffer_size - 6);
			stick_buffer_size -= 6;
		}
		else {
			memmove(stick_buffer, stick_buffer + 1,
			        stick_buffer_size - 1);
			--stick_buffer_size;
		}
	}
}

static void poll_stick(void) {
	if (stick_fd < 0) return;
	unsigned char bytes[64];
	ssize_t count;
	while ((count = read(stick_fd, bytes, sizeof(bytes))) > 0)
		parse_stick_bytes(bytes, (size_t)count);
}

static void init_stick(void) {
	const char *stick_path = "/dev/ttyS2";
	stick_fd =
		open(stick_path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
	if (stick_fd < 0 && errno == ENOENT) {
		// ttyS2 is the A30's physical thumbstick UART. Some environments
		// provide ttyS0 as a compatibility alias, so retain it as a fallback.
		stick_path = "/dev/ttyS0";
		stick_fd =
			open(stick_path,
			     O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
	}
	if (stick_fd < 0) {
		LOG_warn("Could not open A30 analog stick: %s\n", strerror(errno));
		return;
	}
	LOG_info("A30 analog stick opened at %s\n", stick_path);

	struct termios options;
	if (tcgetattr(stick_fd, &options) < 0) {
		LOG_warn("Could not read A30 UART settings: %s\n", strerror(errno));
		close(stick_fd);
		stick_fd = -1;
		return;
	}
	stick_original_termios = options;
	stick_has_original_termios = 1;

	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	options.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
	options.c_cflag |= CS8 | CLOCAL | CREAD;
	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 0;
	tcflush(stick_fd, TCIFLUSH);
	if (tcsetattr(stick_fd, TCSANOW, &options) < 0) {
		LOG_warn("Could not configure A30 analog stick: %s\n",
		         strerror(errno));
		close(stick_fd);
		stick_fd = -1;
		stick_has_original_termios = 0;
	}
}

static int write_text(const char *path, const char *value) {
	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		LOG_warn("Could not open %s: %s\n", path, strerror(errno));
		return -1;
	}
	size_t length = strlen(value);
	ssize_t written = write(fd, value, length);
	close(fd);
	if (written != (ssize_t)length) {
		LOG_warn("Could not write %s: %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

static void write_cpufreq(const char *name, int value) {
	char path[256];
	char text[32];
	snprintf(path, sizeof(path), CPUFREQ_PATH "/%s", name);
	snprintf(text, sizeof(text), "%d", value);
	write_text(path, text);
}

void PLAT_initPlatform(void) {
	// No runtime model variants are currently known for the A30.
}

void PLAT_initInput(void) {
	input_fds[0] =
		open("/dev/input/event0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	input_fds[1] =
		open("/dev/input/event3", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	for (int i = 0; i < MY282_INPUT_COUNT; ++i) {
		if (input_fds[i] < 0)
			LOG_warn("Could not open A30 input %d: %s\n", i,
			         strerror(errno));
	}
	init_stick();
}

void PLAT_quitInput(void) {
	if (stick_fd >= 0) {
		if (stick_has_original_termios)
			tcsetattr(stick_fd, TCSANOW, &stick_original_termios);
		close(stick_fd);
	}
	stick_fd = -1;
	stick_has_original_termios = 0;
	stick_buffer_size = 0;
	stick_x = 0;
	stick_y = 0;

	for (int i = 0; i < MY282_INPUT_COUNT; ++i) {
		if (input_fds[i] >= 0) close(input_fds[i]);
		input_fds[i] = -1;
	}
}

static int map_button(int code, int *id) {
	switch (code) {
	case CODE_UP: *id = BTN_ID_DPAD_UP; return BTN_DPAD_UP;
	case CODE_DOWN: *id = BTN_ID_DPAD_DOWN; return BTN_DPAD_DOWN;
	case CODE_LEFT: *id = BTN_ID_DPAD_LEFT; return BTN_DPAD_LEFT;
	case CODE_RIGHT: *id = BTN_ID_DPAD_RIGHT; return BTN_DPAD_RIGHT;
	case CODE_A: *id = BTN_ID_A; return BTN_A;
	case CODE_B: *id = BTN_ID_B; return BTN_B;
	case CODE_X: *id = BTN_ID_X; return BTN_X;
	case CODE_Y: *id = BTN_ID_Y; return BTN_Y;
	case CODE_START: *id = BTN_ID_START; return BTN_START;
	case CODE_SELECT: *id = BTN_ID_SELECT; return BTN_SELECT;
	case CODE_MENU: *id = BTN_ID_MENU; return BTN_MENU;
	case CODE_L1: *id = BTN_ID_L1; return BTN_L1;
	case CODE_L2: *id = BTN_ID_L2; return BTN_L2;
	case CODE_R1: *id = BTN_ID_R1; return BTN_R1;
	case CODE_R2: *id = BTN_ID_R2; return BTN_R2;
	case CODE_PLUS: *id = BTN_ID_PLUS; return BTN_PLUS;
	case CODE_MINUS: *id = BTN_ID_MINUS; return BTN_MINUS;
	case CODE_POWER: *id = BTN_ID_POWER; return BTN_POWER;
	default: *id = -1; return BTN_NONE;
	}
}

static void update_button(int button, int id, int pressed, uint32_t tick) {
	if (button == BTN_NONE || id < 0) return;

	if (!pressed) {
		if (pad.is_pressed & button) {
			pad.is_pressed &= ~button;
			pad.just_repeated &= ~button;
			pad.just_released |= button;
		}
	}
	else if (!(pad.is_pressed & button)) {
		pad.just_pressed |= button;
		pad.just_repeated |= button;
		pad.is_pressed |= button;
		pad.repeat_at[id] = tick + PAD_REPEAT_DELAY;
	}
}

void PLAT_pollInput(void) {
	pad.just_pressed = BTN_NONE;
	pad.just_released = BTN_NONE;
	pad.just_repeated = BTN_NONE;

	uint32_t tick = SDL_GetTicks();
	for (int id = 0; id < BTN_ID_COUNT; ++id) {
		int button = 1 << id;
		if ((pad.is_pressed & button) && tick >= pad.repeat_at[id]) {
			pad.just_repeated |= button;
			pad.repeat_at[id] += PAD_REPEAT_INTERVAL;
		}
	}

	for (int i = 0; i < MY282_INPUT_COUNT; ++i) {
		if (input_fds[i] < 0) continue;
		struct my282_input_event event;
		while (read(input_fds[i], &event, sizeof(event)) ==
		       (ssize_t)sizeof(event)) {
			if (event.type != MY282_EV_KEY || event.value > 1) continue;
			int id = -1;
			int button = map_button(event.code, &id);
			update_button(button, id, event.value != 0, tick);
		}
	}

	poll_stick();
	pad.laxis.x = stick_x;
	pad.laxis.y = stick_y;
	PAD_setAnalog(BTN_ID_ANALOG_LEFT, BTN_ID_ANALOG_RIGHT, stick_x,
	              tick + PAD_REPEAT_DELAY);
	PAD_setAnalog(BTN_ID_ANALOG_UP, BTN_ID_ANALOG_DOWN, stick_y,
	              tick + PAD_REPEAT_DELAY);
}

int PLAT_shouldWake(void) {
	for (int i = 0; i < MY282_INPUT_COUNT; ++i) {
		if (input_fds[i] < 0) continue;
		struct my282_input_event event;
		while (read(input_fds[i], &event, sizeof(event)) ==
		       (ssize_t)sizeof(event)) {
			if (event.type == MY282_EV_KEY &&
			    event.code == CODE_POWER && event.value == 0)
				return 1;
		}
	}
	return 0;
}

void PLAT_getBatteryStatusFine(int *is_charging, int *charge) {
	if (is_charging) *is_charging = getInt(AC_PATH) == 1;
	if (charge) *charge = clamp_percent(getInt(BATTERY_PATH));
}

void PLAT_getBatteryStatus(int *is_charging, int *charge) {
	int fine_charge = 0;
	PLAT_getBatteryStatusFine(is_charging, &fine_charge);

	if (!charge) return;
	if (fine_charge > 80) *charge = 100;
	else if (fine_charge > 60) *charge = 80;
	else if (fine_charge > 40) *charge = 60;
	else if (fine_charge > 20) *charge = 40;
	else if (fine_charge > 10) *charge = 20;
	else *charge = 10;
}

void PLAT_getCPUTemp(void) {
	int temperature = getInt(CPU_TEMP_PATH);
	// This 3.4 kernel reports degrees C, unlike newer millidegree kernels.
	perf.cpu_temp = temperature > 1000 ? temperature / 1000 : temperature;
}

void PLAT_getCPUSpeed(void) {
	perf.cpu_speed =
		getInt(CPUFREQ_PATH "/scaling_cur_freq") / 1000;
}

void PLAT_getGPUTemp(void) {
	perf.gpu_temp = 0;
}

void PLAT_getGPUSpeed(void) {
	perf.gpu_speed = 0;
}

void PLAT_getGPUUsage(void) {
	perf.gpu_usage = 0;
}

static struct WIFI_connection wifi_connection = {
	.valid = false,
	.freq = -1,
	.link_speed = -1,
	.noise = -1,
	.rssi = -1,
	.ip = {0},
	.ssid = {0},
};

static inline void connection_reset(struct WIFI_connection *connection_info) {
	connection_info->valid = false;
	connection_info->freq = -1;
	connection_info->link_speed = -1;
	connection_info->noise = -1;
	connection_info->rssi = -1;
	connection_info->ip[0] = '\0';
	connection_info->ssid[0] = '\0';
}

void PLAT_getNetworkStatus(int *is_online) {
	if (CFG_getWifi())
		PLAT_wifiConnection(&wifi_connection);
	else
		connection_reset(&wifi_connection);
	if (is_online)
		*is_online = wifi_connection.valid && wifi_connection.ssid[0] != '\0';
}

ConnectionStrength PLAT_connectionStrength(void) {
	if (!CFG_getWifi() || !wifi_connection.valid)
		return SIGNAL_STRENGTH_OFF;
	if (wifi_connection.rssi >= -60) return SIGNAL_STRENGTH_HIGH;
	if (wifi_connection.rssi >= -70) return SIGNAL_STRENGTH_MED;
	return SIGNAL_STRENGTH_LOW;
}

int PLAT_isUSBConnected(void) {
	// The A30 firmware exposes charging state but no UDC state interface.
	return 0;
}

void PLAT_enableBacklight(int enable) {
	if (enable) SetBrightness(GetBrightness());
	else SetRawBrightness(0);
}

void PLAT_powerOff(int reboot) {
	system("rm -f /tmp/nextui_exec");
	sync();

	SetRawVolume(MUTE_VOLUME_RAW);
	PLAT_enableBacklight(0);
	SND_quit();
	VIB_quit();
	PWR_quit();
	GFX_quit();

	touch(reboot ? "/tmp/reboot" : "/tmp/poweroff");
	sync();
	exit(0);
}

int PLAT_supportsDeepSleep(void) {
	return 1;
}

void PLAT_setCPUSpeed(int speed) {
	const char *governor;
	int minimum;
	int maximum;

	switch (speed) {
	case CPU_SPEED_POWERSAVE:
		governor = "conservative";
		minimum = 240000;
		maximum = 1008000;
		break;
	case CPU_SPEED_PERFORMANCE:
		governor = "performance";
		minimum = 1344000;
		maximum = 1344000;
		break;
	case CPU_SPEED_AUTO:
	default:
		governor = "conservative";
		minimum = 648000;
		maximum = 1344000;
		break;
	}

	static const char *controls[] = {
		CPUFREQ_PATH "/scaling_governor",
		CPUFREQ_PATH "/scaling_min_freq",
		CPUFREQ_PATH "/scaling_max_freq",
	};
	mode_t original_modes[sizeof(controls) / sizeof(controls[0])] = {0};
	for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {
		struct stat info;
		if (stat(controls[i], &info) == 0) {
			original_modes[i] = info.st_mode & 0777;
			chmod(controls[i], original_modes[i] | S_IWUSR);
		}
	}

	// Lower min first so transitions from a locked high-frequency profile do
	// not make the kernel reject a lower max. Then apply the final min.
	write_cpufreq("scaling_min_freq", 240000);
	write_cpufreq("scaling_max_freq", maximum);
	write_cpufreq("scaling_min_freq", minimum);
	write_text(CPUFREQ_PATH "/scaling_governor", governor);

	if (speed == CPU_SPEED_AUTO) {
		putInt("/sys/devices/system/cpu/cpufreq/conservative/down_threshold",
		       50);
		putInt("/sys/devices/system/cpu/cpufreq/conservative/up_threshold",
		       80);
		putInt("/sys/devices/system/cpu/cpufreq/conservative/freq_step", 10);
		putInt("/sys/devices/system/cpu/cpufreq/conservative/"
		       "sampling_down_factor", 1);
		putInt("/sys/devices/system/cpu/cpufreq/conservative/sampling_rate",
		       10000);
	}

	for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {
		if (original_modes[i] != 0)
			chmod(controls[i], original_modes[i]);
	}
}

void PLAT_setRumble(int strength) {
	putInt(RUMBLE_PATH, strength ? 1000 : 0);
}

int PLAT_pickSampleRate(int requested, int max) {
	return MIN(requested, max);
}

char *PLAT_getModel(void) {
	return "Miyoo A30";
}

void PLAT_getOsVersionInfo(char *output_str, size_t max_len) {
	if (!output_str || max_len == 0) return;
	output_str[0] = '\0';
	getFile("/usr/miyoo/version", output_str, max_len);
	if (output_str[0] == '\0')
		snprintf(output_str, max_len, "Unknown");
}

#ifndef MY282_PLATFORM_CORE_ONLY
// A30-local SDL presentation shim around the untouched shared UI compositor.
// MinArch's optional shader path still needs separate GLES2 compatibility work.
#include "platform_video.c"

// The generic GL capture flips framebuffer rows for a landscape-native
// display. The A30 renders into an already logical-orientation FBO before its
// portrait scanout rotation, so applying that flip produces an upside-down
// MinArch exit fade. Keep the generic implementation available under a private
// name and provide the A30 capture immediately below.
#define PLAT_GL_screenCapture my282_generic_GL_screenCapture
#include "generic_video.c"
#undef PLAT_GL_screenCapture

unsigned char *PLAT_GL_screenCapture(int *outWidth, int *outHeight) {
	glViewport(0, 0, device_width, device_height);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int width = viewport[2];
	int height = viewport[3];
	if (outWidth) *outWidth = width;
	if (outHeight) *outHeight = height;

	unsigned char *pixels = malloc((size_t)width * height * 4);
	if (!pixels) return NULL;
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	return pixels;
}

// The A30 uses the shared wpa_supplicant backend with a platform-specific
// lifecycle script for its RTL8188FU interface and older BusyBox userspace.
#define WIFI_SOCK_DIR "/tmp/nextui-wifi"
#include "generic_wifi.c"

void PLAT_prepareForProcessExit(void) {
	my282_prepare_for_process_exit();
}
#endif
