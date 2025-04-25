#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "raster/raster_sfx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct rsfx_sound {
    ma_decoder decoder;
    ma_device device;
    int loaded;
    int loop;
    char* path; // For cache lookup
    struct rsfx_sound* next; // For cache linked list
};

static int g_initialized = 0;
static struct rsfx_sound* g_sound_cache = NULL;

// Helper: find sound in cache by path
static struct rsfx_sound* find_cached_sound(const char* path) {
    struct rsfx_sound* s = g_sound_cache;
    while (s) {
        if (s->path && strcmp(s->path, path) == 0) return s;
        s = s->next;
    }
    return NULL;
}

// Helper: add sound to cache
static void cache_sound(struct rsfx_sound* sound) {
    sound->next = g_sound_cache;
    g_sound_cache = sound;
}

// Helper: remove sound from cache (used in clear)
static void remove_sound_from_cache(struct rsfx_sound* sound) {
    struct rsfx_sound** p = &g_sound_cache;
    while (*p) {
        if (*p == sound) {
            *p = sound->next;
            return;
        }
        p = &((*p)->next);
    }
}

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    struct rsfx_sound* s = (struct rsfx_sound*)(((char*)pDevice->pUserData) - offsetof(struct rsfx_sound, decoder));
    if (!pDecoder) return;
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);
    if (framesRead < frameCount) {
        if (s && s->loop) {
            // Loop: seek to start and read remaining frames
            ma_decoder_seek_to_pcm_frame(pDecoder, 0);
            ma_uint64 moreRead = 0;
            ma_decoder_read_pcm_frames(pDecoder, (ma_int16*)pOutput + framesRead * pDecoder->outputChannels, frameCount - framesRead, &moreRead);
            framesRead += moreRead;
        }
        if (framesRead < frameCount) {
            ma_silence_pcm_frames((ma_int16*)pOutput + framesRead * pDecoder->outputChannels,
                                 frameCount - framesRead, pDecoder->outputFormat, pDecoder->outputChannels);
        }
    }
    (void)pInput;
}

bool rsfx_init(void) {
    g_initialized = 1;
    return true;
}

void rsfx_terminate(void) {
    g_initialized = 0;
}

rsfx_sound_t rsfx_load_sound(const char* path) {
    if (!g_initialized) return NULL;
    struct rsfx_sound* cached = find_cached_sound(path);
    if (cached) return cached;
    rsfx_sound_t s = (rsfx_sound_t)calloc(1, sizeof(struct rsfx_sound));
    if (!s) return NULL;
    s->path = strdup(path);
    if (ma_decoder_init_file(path, NULL, &s->decoder) != MA_SUCCESS) {
        free(s->path);
        free(s);
        return NULL;
    }
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = s->decoder.outputFormat;
    deviceConfig.playback.channels = s->decoder.outputChannels;
    deviceConfig.sampleRate        = s->decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &s->decoder;
    if (ma_device_init(NULL, &deviceConfig, &s->device) != MA_SUCCESS) {
        ma_decoder_uninit(&s->decoder);
        free(s->path);
        free(s);
        return NULL;
    }
    s->loaded = 1;
    cache_sound(s);
    return s;
}

void rsfx_free_sound(rsfx_sound_t sound) {
    if (!sound) return;
    if (sound->loaded) {
        ma_device_uninit(&sound->device);
        ma_decoder_uninit(&sound->decoder);
    }
    if (sound->path) free(sound->path);
    remove_sound_from_cache(sound);
    free(sound);
}

void rsfx_clear_cache(void) {
    struct rsfx_sound* s = g_sound_cache;
    while (s) {
        struct rsfx_sound* next = s->next;
        rsfx_free_sound(s);
        s = next;
    }
    g_sound_cache = NULL;
}

bool rsfx_play_sound(rsfx_sound_t sound, bool loop) {
    if (!sound || !sound->loaded) return false;
    sound->loop = loop ? 1 : 0;
    ma_decoder_seek_to_pcm_frame(&sound->decoder, 0);
    ma_device_stop(&sound->device); // Ensure stopped before starting
    return ma_device_start(&sound->device) == MA_SUCCESS;
}

void rsfx_stop_sound(rsfx_sound_t sound) {
    if (!sound || !sound->loaded) return;
    ma_device_stop(&sound->device);
}

void rsfx_set_volume(rsfx_sound_t sound, float volume) {
    if (!sound || !sound->loaded) return;
    ma_device_set_master_volume(&sound->device, volume);
}

#ifdef __EMSCRIPTEN__
void rsfx_resume(void) { /* Not needed for low-level API, but keep for API compatibility */ }
#else
void rsfx_resume(void) { /* Not needed for low-level API, but keep for API compatibility */ }
#endif
