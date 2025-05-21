#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "raster/raster_sfx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct rsfx_sound
{
    ma_decoder         decoder;
    ma_device          device;
    int                loaded;
    int                loop;
    char*              path; // For cache lookup
    struct rsfx_sound* next; // For cache linked list
};

static int           g_sfx_initialized = 0;
static rsfx_sound_t* g_sound_cache     = NULL;

// Helper: find sound in cache by path
static rsfx_sound_t* find_cached_sound(const char* path)
{
    rsfx_sound_t* sound = g_sound_cache;
    while (sound)
    {
        if (sound->path && strcmp(sound->path, path) == 0)
            return sound;
        sound = sound->next;
    }
    return NULL;
}

// Helper: add sound to cache
static void cache_sound(rsfx_sound_t* sound)
{
    sound->next   = g_sound_cache;
    g_sound_cache = sound;
}

// Helper: remove sound from cache (used in clear)
static void remove_sound_from_cache(rsfx_sound_t* sound)
{
    rsfx_sound_t** current = &g_sound_cache;
    while (*current)
    {
        if (*current == sound)
        {
            *current = sound->next;
            break;
        }
        current = &(*current)->next;
    }
}

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (!pDecoder)
        return;

    rsfx_sound_t* sound      = (rsfx_sound_t*)((char*)pDecoder - offsetof(rsfx_sound_t, decoder));
    ma_uint64     framesRead = 0;

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);

    if (framesRead < frameCount && sound && sound->loop)
    {
        ma_decoder_seek_to_pcm_frame(pDecoder, 0);
        ma_uint64 moreRead = 0;
        ma_decoder_read_pcm_frames(
            pDecoder, (ma_int16*)pOutput + framesRead * pDecoder->outputChannels, frameCount - framesRead, &moreRead);
        framesRead += moreRead;
    }

    if (framesRead < frameCount)
    {
        ma_silence_pcm_frames((ma_int16*)pOutput + framesRead * pDecoder->outputChannels,
                              frameCount - framesRead,
                              pDecoder->outputFormat,
                              pDecoder->outputChannels);
    }

    (void)pInput;
}

bool rsfx_init(void)
{
    g_sfx_initialized = 1;
    return true;
}

void rsfx_terminate(void)
{
    g_sfx_initialized = 0;
}

rsfx_sound_t* rsfx_load_sound(const char* path)
{
    if (!g_sfx_initialized)
        return NULL;
    rsfx_sound_t* cached = find_cached_sound(path);
    if (cached)
        return cached;
    rsfx_sound_t* sound = (rsfx_sound_t*)calloc(1, sizeof(rsfx_sound_t));
    if (!sound)
        return NULL;
    sound->path = strdup(path);
    if (ma_decoder_init_file(path, NULL, &sound->decoder) != MA_SUCCESS)
    {
        free(sound->path);
        free(sound);
        return NULL;
    }
    ma_device_config deviceConfig  = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = sound->decoder.outputFormat;
    deviceConfig.playback.channels = sound->decoder.outputChannels;
    deviceConfig.sampleRate        = sound->decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &sound->decoder;
    if (ma_device_init(NULL, &deviceConfig, &sound->device) != MA_SUCCESS)
    {
        ma_decoder_uninit(&sound->decoder);
        free(sound->path);
        free(sound);
        return NULL;
    }
    sound->loaded = 1;
    cache_sound(sound);
    return sound;
}

void rsfx_free_sound(rsfx_sound_t* sound)
{
    if (!sound)
        return;
    if (sound->loaded)
    {
        ma_device_uninit(&sound->device);
        ma_decoder_uninit(&sound->decoder);
    }
    if (sound->path)
        free(sound->path);
    remove_sound_from_cache(sound);
    free(sound);
}

void rsfx_clear_cache(void)
{
    rsfx_sound_t* sound = g_sound_cache;
    while (sound)
    {
        rsfx_sound_t* next = sound->next;
        rsfx_free_sound(sound);
        sound = next;
    }
    g_sound_cache = NULL;
}

bool rsfx_play_sound(rsfx_sound_t* sound, bool loop)
{
    if (!sound || !sound->loaded)
        return false;
    sound->loop = loop ? 1 : 0;
    ma_decoder_seek_to_pcm_frame(&sound->decoder, 0);
    ma_device_stop(&sound->device); // Ensure stopped before starting
    return ma_device_start(&sound->device) == MA_SUCCESS;
}

void rsfx_stop_sound(rsfx_sound_t* sound)
{
    if (!sound || !sound->loaded)
        return;
    ma_device_stop(&sound->device);
}

void rsfx_set_volume(rsfx_sound_t* sound, float volume)
{
    if (!sound || !sound->loaded)
        return;
    ma_device_set_master_volume(&sound->device, volume);
}
