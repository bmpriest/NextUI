#ifndef __msettings_h__
#define __msettings_h__

#define SETTINGS_DEFAULT_BRIGHTNESS 5
#define SETTINGS_DEFAULT_COLORTEMP 20
#define SETTINGS_DEFAULT_CONTRAST 0
#define SETTINGS_DEFAULT_SATURATION 0
#define SETTINGS_DEFAULT_EXPOSURE 0
#define SETTINGS_DEFAULT_VOLUME 8
#define SETTINGS_DEFAULT_HEADPHONE_VOLUME 4
#define SETTINGS_DEFAULT_FAN_SPEED 0

#define SETTINGS_DEFAULT_MUTE_NO_CHANGE -69

void InitSettings(void);
void QuitSettings(void);
int InitializedSettings(void);

int GetBrightness(void);
int GetColortemp(void);
int GetContrast(void);
int GetSaturation(void);
int GetExposure(void);
int GetDisplayCalEnabled(void);
int GetDisplayCalRedGain(void);
int GetDisplayCalGreenGain(void);
int GetDisplayCalBlueGain(void);
int GetVolume(void);

void SetRawBrightness(int value);
void SetRawColortemp(int value);
void SetRawContrast(int value);
void SetRawSaturation(int value);
void SetRawExposure(int value);
void SetRawDisplayCal(int enabled, int red_gain, int green_gain, int blue_gain);
void SetRawVolume(int value);

void SetBrightness(int value);
void SetColortemp(int value);
void SetContrast(int value);
void SetSaturation(int value);
void SetExposure(int value);
void SetDisplayCalEnabled(int value);
void SetDisplayCalRedGain(int value);
void SetDisplayCalGreenGain(int value);
void SetDisplayCalBlueGain(int value);
void SetVolume(int value);

int GetJack(void);
void SetJack(int value);

#define AUDIO_SINK_DEFAULT 0
#define AUDIO_SINK_BLUETOOTH 1
#define AUDIO_SINK_USBDAC 2
int GetAudioSink(void);
void SetAudioSink(int value);

int GetHDMI(void);
void SetHDMI(int value);

int GetMute(void);
void SetMute(int value);

static inline int GetFanSpeed(void) {
	return 0;
}
static inline void SetFanSpeed(int value) {
	(void)value;
}

int GetMutedBrightness(void);
int GetMutedColortemp(void);
int GetMutedContrast(void);
int GetMutedSaturation(void);
int GetMutedExposure(void);
int GetMutedVolume(void);
int GetMuteDisablesDpad(void);
int GetMuteEmulatesJoystick(void);
int GetMuteTurboA(void);
int GetMuteTurboB(void);
int GetMuteTurboX(void);
int GetMuteTurboY(void);
int GetMuteTurboL1(void);
int GetMuteTurboL2(void);
int GetMuteTurboR1(void);
int GetMuteTurboR2(void);

void SetMutedBrightness(int value);
void SetMutedColortemp(int value);
void SetMutedContrast(int value);
void SetMutedSaturation(int value);
void SetMutedExposure(int value);
void SetMutedVolume(int value);
void SetMuteDisablesDpad(int value);
void SetMuteEmulatesJoystick(int value);
void SetMuteTurboA(int value);
void SetMuteTurboB(int value);
void SetMuteTurboX(int value);
void SetMuteTurboY(int value);
void SetMuteTurboL1(int value);
void SetMuteTurboL2(int value);
void SetMuteTurboR1(int value);
void SetMuteTurboR2(int value);

#endif
