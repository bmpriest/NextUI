// RetroAchievements-disabled implementation for early platform bring-up.
//
// The unified MinArch menu references the RA interfaces even on platforms
// where achievements are not linked. These stubs keep those paths inert until
// the my282 Wi-Fi and lifecycle work is ready.

#include "ra_integration.h"
#include "ra_badges.h"

void RA_init(void) {}
void RA_quit(void) {}
void RA_loadGame(
	const char *rom_path, const uint8_t *rom_data, size_t rom_size,
	const char *emu_tag
) {
	(void)rom_path;
	(void)rom_data;
	(void)rom_size;
	(void)emu_tag;
}
void RA_unloadGame(void) {}
void RA_doFrame(void) {}
void RA_idle(void) {}
bool RA_isGameLoaded(void) { return false; }
bool RA_isHardcoreModeActive(void) { return false; }
bool RA_isLoggedIn(void) { return false; }
const char *RA_getUserDisplayName(void) { return NULL; }
const char *RA_getGameTitle(void) { return NULL; }
void RA_getAchievementSummary(uint32_t *unlocked, uint32_t *total) {
	if (unlocked) *unlocked = 0;
	if (total) *total = 0;
}
const void *RA_createAchievementList(int category, int grouping) {
	(void)category;
	(void)grouping;
	return NULL;
}
void RA_destroyAchievementList(const void *list) { (void)list; }
const char *RA_getGameHash(void) { return NULL; }
bool RA_isAchievementMuted(uint32_t achievement_id) {
	(void)achievement_id;
	return false;
}
bool RA_toggleAchievementMute(uint32_t achievement_id) {
	(void)achievement_id;
	return false;
}
void RA_setAchievementMuted(uint32_t achievement_id, bool muted) {
	(void)achievement_id;
	(void)muted;
}
bool RA_isAchievementOfflinePending(uint32_t achievement_id) {
	(void)achievement_id;
	return false;
}
void RA_setMemoryAccessors(
	RA_GetMemoryFunc get_memory_data, RA_GetMemorySizeFunc get_memory_size
) {
	(void)get_memory_data;
	(void)get_memory_size;
}
void RA_setMemoryMap(const void *mmap) { (void)mmap; }
void RA_initMemoryRegions(uint32_t console_id) { (void)console_id; }

void RA_Badges_init(void) {}
void RA_Badges_quit(void) {}
void RA_Badges_clearMemory(void) {}
void RA_Badges_prefetch(const char **badge_names, size_t count) {
	(void)badge_names;
	(void)count;
}
void RA_Badges_prefetchOne(const char *badge_name, bool locked) {
	(void)badge_name;
	(void)locked;
}
SDL_Surface *RA_Badges_get(const char *badge_name, bool locked) {
	(void)badge_name;
	(void)locked;
	return NULL;
}
SDL_Surface *RA_Badges_getNotificationSize(
	const char *badge_name, bool locked
) {
	(void)badge_name;
	(void)locked;
	return NULL;
}
RA_BadgeState RA_Badges_getState(const char *badge_name, bool locked) {
	(void)badge_name;
	(void)locked;
	return RA_BADGE_STATE_UNKNOWN;
}
void RA_Badges_getCachePath(
	const char *badge_name, bool locked, char *buffer, size_t buffer_size
) {
	(void)badge_name;
	(void)locked;
	if (buffer && buffer_size) buffer[0] = '\0';
}
void RA_Badges_getUrl(
	const char *badge_name, bool locked, char *buffer, size_t buffer_size
) {
	(void)badge_name;
	(void)locked;
	if (buffer && buffer_size) buffer[0] = '\0';
}
