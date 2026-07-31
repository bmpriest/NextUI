#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <linux/input.h>
#include <msettings.h>

#define VOLUME_MIN 0
#define VOLUME_MAX 20
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 10

#define CODE_MENU 1
#define CODE_PLUS 115
#define CODE_MINUS 114

#define INPUT_COUNT 2
#define REPEAT_DELAY 300
#define REPEAT_INTERVAL 100

static const char *input_paths[INPUT_COUNT] = {
	"/dev/input/event0",
	"/dev/input/event3",
};
static int input_fds[INPUT_COUNT] = {-1, -1};
static volatile sig_atomic_t quit;

static void on_term(int signal_number) {
	(void)signal_number;
	quit = 1;
}

static uint32_t ticks(void) {
	struct timeval now;
	gettimeofday(&now, NULL);
	return (uint32_t)(now.tv_sec * 1000U + now.tv_usec / 1000U);
}

static void adjust_setting(int direction, int menu_pressed) {
	if (menu_pressed) {
		int value = GetBrightness() + direction;
		if (value < BRIGHTNESS_MIN) value = BRIGHTNESS_MIN;
		if (value > BRIGHTNESS_MAX) value = BRIGHTNESS_MAX;
		SetBrightness(value);
	}
	else {
		int value = GetVolume() + direction;
		if (value < VOLUME_MIN) value = VOLUME_MIN;
		if (value > VOLUME_MAX) value = VOLUME_MAX;
		SetVolume(value);
	}
}

int main(void) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = on_term;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);

	InitSettings();
	if (!InitializedSettings()) {
		fprintf(stderr, "keymon: could not initialize shared settings\n");
		return 1;
	}

	for (int i = 0; i < INPUT_COUNT; ++i) {
		input_fds[i] =
			open(input_paths[i], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
		if (input_fds[i] < 0)
			fprintf(stderr, "keymon: open %s: %s\n", input_paths[i],
			        strerror(errno));
	}

	int menu_pressed = 0;
	int plus_pressed = 0;
	int minus_pressed = 0;
	uint32_t plus_repeat_at = 0;
	uint32_t minus_repeat_at = 0;

	while (!quit) {
		uint32_t now = ticks();

		for (int i = 0; i < INPUT_COUNT; ++i) {
			if (input_fds[i] < 0) continue;

			struct input_event event;
			while (read(input_fds[i], &event, sizeof(event)) ==
			       (ssize_t)sizeof(event)) {
				if (event.type != EV_KEY || event.value > 1) continue;

				switch (event.code) {
				case CODE_MENU:
					menu_pressed = event.value;
					break;
				case CODE_PLUS:
					plus_pressed = event.value;
					if (event.value) {
						adjust_setting(1, menu_pressed);
						plus_repeat_at = now + REPEAT_DELAY;
					}
					break;
				case CODE_MINUS:
					minus_pressed = event.value;
					if (event.value) {
						adjust_setting(-1, menu_pressed);
						minus_repeat_at = now + REPEAT_DELAY;
					}
					break;
				default:
					break;
				}
			}
		}

		if (plus_pressed && now >= plus_repeat_at) {
			adjust_setting(1, menu_pressed);
			plus_repeat_at += REPEAT_INTERVAL;
		}
		if (minus_pressed && now >= minus_repeat_at) {
			adjust_setting(-1, menu_pressed);
			minus_repeat_at += REPEAT_INTERVAL;
		}

		usleep(10000);
	}

	for (int i = 0; i < INPUT_COUNT; ++i)
		if (input_fds[i] >= 0) close(input_fds[i]);
	QuitSettings();
	return 0;
}
