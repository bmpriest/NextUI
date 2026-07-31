#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include "msettings.h"

#define SETTINGS_VERSION 11
#define SHM_KEY "/SharedSettings"
#define BRIGHTNESS_PATH "/sys/devices/virtual/disp/disp/attr/lcdbl"

typedef struct Settings {
	int version;
	int brightness;
	int colortemperature;
	int headphones;
	int speaker;
	int mute;
	int contrast;
	int saturation;
	int exposure;
	int toggled_brightness;
	int toggled_colortemperature;
	int toggled_contrast;
	int toggled_saturation;
	int toggled_exposure;
	int toggled_volume;
	int disable_dpad_on_mute;
	int emulate_joystick_on_mute;
	int turbo_a;
	int turbo_b;
	int turbo_x;
	int turbo_y;
	int turbo_l1;
	int turbo_l2;
	int turbo_r1;
	int turbo_r2;
	int unused[2];
	int jack;
	int audiosink;
	int displaycal_enabled;
	int displaycal_red_gain;
	int displaycal_green_gain;
	int displaycal_blue_gain;
	int hdmi;
} Settings;

typedef struct LegacySettingsV1 {
	int version;
	int brightness;
	int headphones;
	int speaker;
	int unused[2];
	int jack;
	int hdmi;
} LegacySettingsV1;

static const Settings defaults = {
	.version = SETTINGS_VERSION,
	.brightness = SETTINGS_DEFAULT_BRIGHTNESS,
	.colortemperature = SETTINGS_DEFAULT_COLORTEMP,
	.headphones = SETTINGS_DEFAULT_HEADPHONE_VOLUME,
	.speaker = SETTINGS_DEFAULT_VOLUME,
	.contrast = SETTINGS_DEFAULT_CONTRAST,
	.saturation = SETTINGS_DEFAULT_SATURATION,
	.exposure = SETTINGS_DEFAULT_EXPOSURE,
	.toggled_brightness = SETTINGS_DEFAULT_MUTE_NO_CHANGE,
	.toggled_colortemperature = SETTINGS_DEFAULT_MUTE_NO_CHANGE,
	.toggled_contrast = SETTINGS_DEFAULT_MUTE_NO_CHANGE,
	.toggled_saturation = SETTINGS_DEFAULT_MUTE_NO_CHANGE,
	.toggled_exposure = SETTINGS_DEFAULT_MUTE_NO_CHANGE,
	.displaycal_red_gain = 100,
	.displaycal_green_gain = 100,
	.displaycal_blue_gain = 100,
};

static const int brightness_raw[11] = {
	2, 8, 18, 32, 50, 72, 98, 128, 162, 200, 255,
};

static const int volume_raw[21] = {
	0, 12, 25, 38, 51, 63, 78, 89, 102, 114, 127,
	140, 153, 165, 178, 191, 204, 216, 229, 242, 255,
};

static Settings *settings;
static int shm_fd = -1;
static int is_host;
static char settings_path[512];

static int clamp(int value, int low, int high) {
	if (value < low) return low;
	if (value > high) return high;
	return value;
}

static void save_settings(void) {
	if (!settings || settings_path[0] == '\0') return;

	int fd = open(settings_path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) {
		fprintf(stderr, "msettings: open %s: %s\n", settings_path,
		        strerror(errno));
		return;
	}
	ssize_t written = write(fd, settings, sizeof(*settings));
	if (written != (ssize_t)sizeof(*settings))
		fprintf(stderr, "msettings: short write to %s\n", settings_path);
	fsync(fd);
	close(fd);
}

static void load_settings(void) {
	memcpy(settings, &defaults, sizeof(*settings));

	int fd = open(settings_path, O_RDONLY);
	if (fd < 0) return;

	int version = 0;
	if (read(fd, &version, sizeof(version)) != sizeof(version)) {
		close(fd);
		return;
	}
	lseek(fd, 0, SEEK_SET);

	if (version == SETTINGS_VERSION) {
		Settings loaded;
		if (read(fd, &loaded, sizeof(loaded)) == sizeof(loaded))
			memcpy(settings, &loaded, sizeof(*settings));
	}
	else if (version == 1) {
		LegacySettingsV1 legacy;
		if (read(fd, &legacy, sizeof(legacy)) == sizeof(legacy)) {
			settings->brightness = clamp(legacy.brightness, 0, 10);
			settings->headphones = clamp(legacy.headphones, 0, 20);
			settings->speaker = clamp(legacy.speaker, 0, 20);
		}
	}
	else {
		fprintf(stderr, "msettings: ignoring unsupported version %d\n",
		        version);
	}
	close(fd);
}

void InitSettings(void) {
	if (settings) return;

	const char *userdata = getenv("USERDATA_PATH");
	if (userdata && userdata[0] != '\0')
		snprintf(settings_path, sizeof(settings_path), "%s/msettings.bin",
		         userdata);

	shm_fd = shm_open(SHM_KEY, O_RDWR | O_CREAT | O_EXCL, 0644);
	if (shm_fd < 0 && errno == EEXIST) {
		shm_fd = shm_open(SHM_KEY, O_RDWR, 0644);
	}
	else if (shm_fd >= 0) {
		is_host = 1;
		if (ftruncate(shm_fd, sizeof(*settings)) < 0)
			fprintf(stderr, "msettings: ftruncate: %s\n", strerror(errno));
	}

	if (shm_fd < 0) {
		fprintf(stderr, "msettings: shm_open: %s\n", strerror(errno));
		return;
	}

	settings = mmap(NULL, sizeof(*settings), PROT_READ | PROT_WRITE,
	                MAP_SHARED, shm_fd, 0);
	if (settings == MAP_FAILED) {
		fprintf(stderr, "msettings: mmap: %s\n", strerror(errno));
		settings = NULL;
		close(shm_fd);
		shm_fd = -1;
		return;
	}

	if (is_host) load_settings();
	SetBrightness(GetBrightness());
	SetVolume(GetVolume());
}

int InitializedSettings(void) {
	return settings != NULL;
}

void QuitSettings(void) {
	if (!settings) return;
	munmap(settings, sizeof(*settings));
	settings = NULL;
	close(shm_fd);
	shm_fd = -1;
	if (is_host) shm_unlink(SHM_KEY);
	is_host = 0;
}

int GetBrightness(void) { return settings ? settings->brightness : 5; }
int GetColortemp(void) { return settings ? settings->colortemperature : 20; }
int GetContrast(void) { return settings ? settings->contrast : 0; }
int GetSaturation(void) { return settings ? settings->saturation : 0; }
int GetExposure(void) { return settings ? settings->exposure : 0; }
int GetDisplayCalEnabled(void) {
	return settings ? settings->displaycal_enabled : 0;
}
int GetDisplayCalRedGain(void) {
	return settings ? settings->displaycal_red_gain : 100;
}
int GetDisplayCalGreenGain(void) {
	return settings ? settings->displaycal_green_gain : 100;
}
int GetDisplayCalBlueGain(void) {
	return settings ? settings->displaycal_blue_gain : 100;
}
int GetVolume(void) {
	if (!settings) return SETTINGS_DEFAULT_VOLUME;
	return (settings->jack || settings->audiosink != AUDIO_SINK_DEFAULT)
		? settings->headphones
		: settings->speaker;
}
int GetJack(void) { return settings ? settings->jack : 0; }
int GetAudioSink(void) {
	return settings ? settings->audiosink : AUDIO_SINK_DEFAULT;
}
int GetHDMI(void) { return settings ? settings->hdmi : 0; }
int GetMute(void) { return settings ? settings->mute : 0; }

void SetRawBrightness(int value) {
	int fd = open(BRIGHTNESS_PATH, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "msettings: open brightness: %s\n", strerror(errno));
		return;
	}
	char buffer[16];
	int length = snprintf(buffer, sizeof(buffer), "%d", clamp(value, 0, 255));
	if (write(fd, buffer, length) != length)
		fprintf(stderr, "msettings: write brightness: %s\n", strerror(errno));
	close(fd);
}

void SetRawVolume(int value) {
	/*
	 * Use the A30 codec's hardware volume control. `Soft Volume Master` is a
	 * user-created softvol control and can be owned by the process that opened
	 * the PCM device; keymon then receives EPERM while a game is running.
	 * Allium uses this hardware element on A30 as it drives both the speaker
	 * and headphone output.
	 */
	snd_ctl_t *control = NULL;
	snd_ctl_elem_id_t *id = NULL;
	snd_ctl_elem_info_t *info = NULL;
	snd_ctl_elem_value_t *element = NULL;
	int error = snd_ctl_open(&control, "default", 0);
	if (error < 0) goto fail;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, "headphone volume");
	snd_ctl_elem_id_set_index(id, 0);

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_info_set_id(info, id);
	if ((error = snd_ctl_elem_info(control, info)) < 0) goto fail;
	if (snd_ctl_elem_info_get_type(info) != SND_CTL_ELEM_TYPE_INTEGER) {
		error = -EINVAL;
		goto fail;
	}

	long minimum = snd_ctl_elem_info_get_min(info);
	long maximum = snd_ctl_elem_info_get_max(info);
	long raw = minimum +
		(long)clamp(value, 0, 255) * (maximum - minimum) / 255;

	snd_ctl_elem_value_alloca(&element);
	snd_ctl_elem_value_set_id(element, id);
	unsigned int count = snd_ctl_elem_info_get_count(info);
	for (unsigned int channel = 0; channel < count; ++channel)
		snd_ctl_elem_value_set_integer(element, channel, raw);
	if ((error = snd_ctl_elem_write(control, element)) < 0) goto fail;

	snd_ctl_close(control);
	return;

fail:
	fprintf(stderr, "msettings: failed to set ALSA volume: %s\n",
	        snd_strerror(error));
	if (control) snd_ctl_close(control);
}

void SetBrightness(int value) {
	if (!settings) return;
	value = clamp(value, 0, 10);
	settings->brightness = value;
	SetRawBrightness(brightness_raw[value]);
	save_settings();
}

void SetVolume(int value) {
	if (!settings) return;
	value = clamp(value, 0, 20);
	if (settings->jack) settings->headphones = value;
	else settings->speaker = value;
	if (!settings->mute) SetRawVolume(volume_raw[value]);
	save_settings();
}

void SetJack(int value) {
	if (!settings) return;
	settings->jack = !!value;
	SetVolume(GetVolume());
}

void SetAudioSink(int value) {
	if (!settings) return;
	settings->audiosink = value;
	SetVolume(GetVolume());
}

void SetHDMI(int value) {
	if (!settings) return;
	settings->hdmi = !!value;
	save_settings();
}

void SetMute(int value) {
	if (!settings) return;
	settings->mute = !!value;
	SetRawVolume(settings->mute ? 0 : volume_raw[GetVolume()]);
	save_settings();
}

#define SIMPLE_GETTER(name, field, fallback) \
	int name(void) { return settings ? settings->field : fallback; }
#define SIMPLE_SETTER(name, field) \
	void name(int value) { \
		if (!settings) return; \
		settings->field = value; \
		save_settings(); \
	}

SIMPLE_GETTER(GetMutedBrightness, toggled_brightness,
              SETTINGS_DEFAULT_MUTE_NO_CHANGE)
SIMPLE_GETTER(GetMutedColortemp, toggled_colortemperature,
              SETTINGS_DEFAULT_MUTE_NO_CHANGE)
SIMPLE_GETTER(GetMutedContrast, toggled_contrast,
              SETTINGS_DEFAULT_MUTE_NO_CHANGE)
SIMPLE_GETTER(GetMutedSaturation, toggled_saturation,
              SETTINGS_DEFAULT_MUTE_NO_CHANGE)
SIMPLE_GETTER(GetMutedExposure, toggled_exposure,
              SETTINGS_DEFAULT_MUTE_NO_CHANGE)
SIMPLE_GETTER(GetMutedVolume, toggled_volume, 0)
SIMPLE_GETTER(GetMuteDisablesDpad, disable_dpad_on_mute, 0)
SIMPLE_GETTER(GetMuteEmulatesJoystick, emulate_joystick_on_mute, 0)
SIMPLE_GETTER(GetMuteTurboA, turbo_a, 0)
SIMPLE_GETTER(GetMuteTurboB, turbo_b, 0)
SIMPLE_GETTER(GetMuteTurboX, turbo_x, 0)
SIMPLE_GETTER(GetMuteTurboY, turbo_y, 0)
SIMPLE_GETTER(GetMuteTurboL1, turbo_l1, 0)
SIMPLE_GETTER(GetMuteTurboL2, turbo_l2, 0)
SIMPLE_GETTER(GetMuteTurboR1, turbo_r1, 0)
SIMPLE_GETTER(GetMuteTurboR2, turbo_r2, 0)

SIMPLE_SETTER(SetMutedBrightness, toggled_brightness)
SIMPLE_SETTER(SetMutedColortemp, toggled_colortemperature)
SIMPLE_SETTER(SetMutedContrast, toggled_contrast)
SIMPLE_SETTER(SetMutedSaturation, toggled_saturation)
SIMPLE_SETTER(SetMutedExposure, toggled_exposure)
SIMPLE_SETTER(SetMutedVolume, toggled_volume)
SIMPLE_SETTER(SetMuteDisablesDpad, disable_dpad_on_mute)
SIMPLE_SETTER(SetMuteEmulatesJoystick, emulate_joystick_on_mute)
SIMPLE_SETTER(SetMuteTurboA, turbo_a)
SIMPLE_SETTER(SetMuteTurboB, turbo_b)
SIMPLE_SETTER(SetMuteTurboX, turbo_x)
SIMPLE_SETTER(SetMuteTurboY, turbo_y)
SIMPLE_SETTER(SetMuteTurboL1, turbo_l1)
SIMPLE_SETTER(SetMuteTurboL2, turbo_l2)
SIMPLE_SETTER(SetMuteTurboR1, turbo_r1)
SIMPLE_SETTER(SetMuteTurboR2, turbo_r2)

void SetColortemp(int value) {
	if (!settings) return;
	settings->colortemperature = value;
	save_settings();
}
void SetContrast(int value) {
	if (!settings) return;
	settings->contrast = value;
	save_settings();
}
void SetSaturation(int value) {
	if (!settings) return;
	settings->saturation = value;
	save_settings();
}
void SetExposure(int value) {
	if (!settings) return;
	settings->exposure = value;
	save_settings();
}
void SetDisplayCalEnabled(int value) {
	if (!settings) return;
	settings->displaycal_enabled = !!value;
	save_settings();
}
void SetDisplayCalRedGain(int value) {
	if (!settings) return;
	settings->displaycal_red_gain = value;
	save_settings();
}
void SetDisplayCalGreenGain(int value) {
	if (!settings) return;
	settings->displaycal_green_gain = value;
	save_settings();
}
void SetDisplayCalBlueGain(int value) {
	if (!settings) return;
	settings->displaycal_blue_gain = value;
	save_settings();
}

void SetRawColortemp(int value) { (void)value; }
void SetRawContrast(int value) { (void)value; }
void SetRawSaturation(int value) { (void)value; }
void SetRawExposure(int value) { (void)value; }
void SetRawDisplayCal(int enabled, int red_gain, int green_gain,
                      int blue_gain) {
	(void)enabled;
	(void)red_gain;
	(void)green_gain;
	(void)blue_gain;
}
