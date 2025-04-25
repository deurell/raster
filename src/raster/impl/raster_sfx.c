#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "raster/raster_sfx.h"
#include <stdlib.h>
#include <string.h>

// Audio engine context
static ma_engine g_engine;
static bool g_engine_initialized = false;

struct rsfx_sound {
    ma_sound sound;
};

bool rsfx_init(void) {
    if (g_engine_initialized) return true;
    ma_result result = ma_engine_init(NULL, &g_engine);
    if (result != MA_SUCCESS) return false;
    g_engine_initialized = true;
    return true;
}

void rsfx_terminate(void) {
    if (!g_engine_initialized) return;
    ma_engine_uninit(&g_engine);
    g_engine_initialized = false;
}

rsfx_sound_t rsfx_load_sound(const char* path) {
    if (!g_engine_initialized) return NULL;
    rsfx_sound_t s = (rsfx_sound_t)malloc(sizeof(struct rsfx_sound));
    if (!s) return NULL;
    ma_result result = ma_sound_init_from_file(&g_engine, path, 0, NULL, NULL, &s->sound);
    if (result != MA_SUCCESS) {
        free(s);
        return NULL;
    }
    return s;
}

void rsfx_free_sound(rsfx_sound_t sound) {
    if (!sound) return;
    ma_sound_uninit(&sound->sound);
    free(sound);
}

bool rsfx_play_sound(rsfx_sound_t sound, bool loop) {
    if (!sound) return false;
    ma_sound_set_looping(&sound->sound, loop ? MA_TRUE : MA_FALSE);
    return ma_sound_start(&sound->sound) == MA_SUCCESS;
}

void rsfx_stop_sound(rsfx_sound_t sound) {
    if (!sound) return;
    ma_sound_stop(&sound->sound);
}

void rsfx_set_volume(rsfx_sound_t sound, float volume) {
    if (!sound) return;
    ma_sound_set_volume(&sound->sound, volume);
}
