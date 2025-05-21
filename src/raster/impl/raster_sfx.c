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
    rsfx_sound_t* s = g_sound_cache;
    while (s)
    {
        if (s->path && strcmp(s->path, path) == 0)
            return s;
        s = s->next;
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
    rsfx_sound_t* s          = (rsfx_sound_t*)((char*)pDecoder - offsetof(struct rsfx_sound, decoder));
    ma_uint64     framesRead = 0;
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);
    if (framesRead < frameCount)
    {
        if (s && s->loop)
        {
            // Loop: seek to start and read remaining frames
            ma_decoder_seek_to_pcm_frame(pDecoder, 0);
            ma_uint64 moreRead = 0;
            ma_decoder_read_pcm_frames(pDecoder,
                                       (ma_int16*)pOutput + framesRead * pDecoder->outputChannels,
                                       frameCount - framesRead,
                                       &moreRead);
            framesRead += moreRead;
        }
        if (framesRead < frameCount)
        {
            ma_silence_pcm_frames((ma_int16*)pOutput + framesRead * pDecoder->outputChannels,
                                  frameCount - framesRead,
                                  pDecoder->outputFormat,
                                  pDecoder->outputChannels);
        }
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
    rsfx_sound_t* s = (rsfx_sound_t*)calloc(1, sizeof(rsfx_sound_t));
    if (!s)
        return NULL;
    s->path = strdup(path);
    if (ma_decoder_init_file(path, NULL, &s->decoder) != MA_SUCCESS)
    {
        free(s->path);
        free(s);
        return NULL;
    }
    ma_device_config deviceConfig  = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = s->decoder.outputFormat;
    deviceConfig.playback.channels = s->decoder.outputChannels;
    deviceConfig.sampleRate        = s->decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &s->decoder;
    if (ma_device_init(NULL, &deviceConfig, &s->device) != MA_SUCCESS)
    {
        ma_decoder_uninit(&s->decoder);
        free(s->path);
        free(s);
        return NULL;
    }
    s->loaded = 1;
    cache_sound(s);
    return s;
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
    rsfx_sound_t* s = g_sound_cache;
    while (s)
    {
        rsfx_sound_t* next = s->next;
        rsfx_free_sound(s);
        s = next;
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
