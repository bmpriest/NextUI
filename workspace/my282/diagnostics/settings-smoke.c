#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <msettings.h>

#define BRIGHTNESS_PATH "/sys/devices/virtual/disp/disp/attr/lcdbl"

static int read_int(const char *path, int *value) {
	FILE *file = fopen(path, "r");
	if (!file) return 0;
	int matched = fscanf(file, "%d", value);
	fclose(file);
	return matched == 1;
}

static int read_volume(int *value) {
	FILE *amixer = popen("amixer get 'Soft Volume Master'", "r");
	if (!amixer) return 0;

	int found = 0;
	char line[256];
	while (fgets(line, sizeof(line), amixer)) {
		char *channel = strstr(line, "Front Left:");
		if (channel && sscanf(channel, "Front Left: %d", value) == 1) {
			found = 1;
			break;
		}
	}
	pclose(amixer);
	return found;
}

int main(void) {
	int brightness_before = 0;
	int brightness_after = 0;
	int volume_before = 0;
	int volume_after = 0;

	if (!read_int(BRIGHTNESS_PATH, &brightness_before)) {
		fprintf(stderr, "could not read brightness\n");
		return 1;
	}
	if (!read_volume(&volume_before)) {
		fprintf(stderr, "could not read Soft Volume Master\n");
		return 1;
	}

	// Exercise the real production accessors without changing either setting.
	SetRawBrightness(brightness_before);
	SetRawVolume(volume_before);

	if (!read_int(BRIGHTNESS_PATH, &brightness_after) ||
	    !read_volume(&volume_after)) {
		fprintf(stderr, "could not read settings after round-trip\n");
		return 1;
	}

	printf("brightness raw: %d -> %d\n", brightness_before,
	       brightness_after);
	printf("Soft Volume Master raw: %d -> %d\n", volume_before,
	       volume_after);
	return brightness_before == brightness_after &&
		       volume_before == volume_after ? 0 : 1;
}
