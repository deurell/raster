#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "raster/raster_sfx.h"
#include "raster/raster_log.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct rsfx_sound
{
    ma_decoder       decoder;
    ma_device        device;
    int              loaded;
    int              loop;
    char*            path;
    rsfx_sound_handle handle;
    struct rsfx_sound* next; // cache linked list
} rsfx_sound_t;

#define RSFX_MAX_SOUNDS 128u
#define RSFX_HANDLE_INDEX_MASK 0xFFFFu
#define RSFX_HANDLE_GENERATION_SHIFT 16u

typedef struct
{
    rsfx_sound_t* object;
    uint32_t      generation;
} rsfx_sound_slot_t;

static int             g_sfx_initialized      = 0;
static bool            g_sound_pool_initialized = false;
static rsfx_sound_slot_t g_sound_slots[RSFX_MAX_SOUNDS];
static uint32_t        g_sound_free_stack[RSFX_MAX_SOUNDS];
static uint32_t        g_sound_free_top = 0;
static rsfx_sound_t*   g_sound_cache    = NULL;

static void rsfx_initialize_sound_pool(void)
{
    if (g_sound_pool_initialized)
    {
        return;
    }

    for (uint32_t i = 0; i < RSFX_MAX_SOUNDS; ++i)
    {
        g_sound_slots[i].object     = NULL;
        g_sound_slots[i].generation = 1u;
        g_sound_free_stack[g_sound_free_top++] = (RSFX_MAX_SOUNDS - 1u) - i;
    }

    g_sound_pool_initialized = true;
}

static inline uint32_t rsfx_handle_index(uint32_t handle)
{
    return (handle & RSFX_HANDLE_INDEX_MASK) - 1u;
}

static inline uint32_t rsfx_handle_generation(uint32_t handle)
{
    return handle >> RSFX_HANDLE_GENERATION_SHIFT;
}

static inline uint32_t rsfx_make_handle(uint32_t index, uint32_t generation)
{
    return ((generation & RSFX_HANDLE_INDEX_MASK) << RSFX_HANDLE_GENERATION_SHIFT) | (index + 1u);
}

static inline uint32_t rsfx_next_generation(uint32_t generation)
{
    generation = (generation + 1u) & RSFX_HANDLE_INDEX_MASK;
    if (generation == 0u)
    {
        generation = 1u;
    }
    return generation;
}

static rsfx_sound_slot_t* rsfx_sound_slot_from_handle(rsfx_sound_handle handle, uint32_t* out_index)
{
    if (handle == RSFX_INVALID_SOUND_HANDLE)
    {
        return NULL;
    }

    uint32_t index = rsfx_handle_index(handle);
    if (index >= RSFX_MAX_SOUNDS)
    {
        return NULL;
    }

    rsfx_sound_slot_t* slot = &g_sound_slots[index];
    if (slot->object == NULL)
    {
        return NULL;
    }

    if (slot->generation != rsfx_handle_generation(handle))
    {
        return NULL;
    }

    if (out_index)
    {
        *out_index = index;
    }

    return slot;
}

static rsfx_sound_t* rsfx_sound_from_handle(rsfx_sound_handle handle)
{
    rsfx_initialize_sound_pool();
    rsfx_sound_slot_t* slot = rsfx_sound_slot_from_handle(handle, NULL);
    return slot ? slot->object : NULL;
}

static rsfx_sound_handle rsfx_sound_register(rsfx_sound_t* sound)
{
    rsfx_initialize_sound_pool();

    if (g_sound_free_top == 0)
    {
        rlog_error("rsfx: sound pool exhausted (max %u)", (unsigned)RSFX_MAX_SOUNDS);
        return RSFX_INVALID_SOUND_HANDLE;
    }

    uint32_t index = g_sound_free_stack[--g_sound_free_top];
    rsfx_sound_slot_t* slot = &g_sound_slots[index];
    slot->object = sound;

    rsfx_sound_handle handle = rsfx_make_handle(index, slot->generation);
    sound->handle = handle;
    return handle;
}

static void rsfx_sound_unregister(rsfx_sound_handle handle)
{
    uint32_t index = 0;
    rsfx_sound_slot_t* slot = rsfx_sound_slot_from_handle(handle, &index);
    if (!slot)
    {
        return;
    }

    slot->object     = NULL;
    slot->generation = rsfx_next_generation(slot->generation);
    g_sound_free_stack[g_sound_free_top++] = index;
}

static rsfx_sound_t* find_cached_sound(const char* path)
{
    rsfx_sound_t* sound = g_sound_cache;
    while (sound)
    {
        if (sound->path && strcmp(sound->path, path) == 0)
        {
            return sound;
        }
        sound = sound->next;
    }
    return NULL;
}

static void cache_sound(rsfx_sound_t* sound)
{
    sound->next   = g_sound_cache;
    g_sound_cache = sound;
}

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

    rsfx_sound_t* sound = (rsfx_sound_t*)((char*)pDecoder - offsetof(rsfx_sound_t, decoder));
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
    rsfx_initialize_sound_pool();
    g_sfx_initialized = 1;
    return true;
}

rsfx_sound_handle rsfx_load_sound(const char* path)
{
    if (!g_sfx_initialized)
        return RSFX_INVALID_SOUND_HANDLE;
    rsfx_sound_t* cached = find_cached_sound(path);
    if (cached)
        return cached->handle;
    rsfx_sound_t* sound = (rsfx_sound_t*)calloc(1, sizeof(rsfx_sound_t));
    if (!sound)
        return RSFX_INVALID_SOUND_HANDLE;
    sound->path = strdup(path);
    if (ma_decoder_init_file(path, NULL, &sound->decoder) != MA_SUCCESS)
    {
        free(sound->path);
        free(sound);
        return RSFX_INVALID_SOUND_HANDLE;
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
        return RSFX_INVALID_SOUND_HANDLE;
    }
    sound->loaded = 1;
    cache_sound(sound);
    rsfx_sound_handle handle = rsfx_sound_register(sound);
    if (handle == RSFX_INVALID_SOUND_HANDLE)
    {
        ma_device_uninit(&sound->device);
        ma_decoder_uninit(&sound->decoder);
        free(sound->path);
        free(sound);
        return RSFX_INVALID_SOUND_HANDLE;
    }

    return handle;
}

void rsfx_free_sound(rsfx_sound_handle handle)
{
    rsfx_sound_t* sound = rsfx_sound_from_handle(handle);
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
    rsfx_sound_unregister(handle);
    free(sound);
}

void rsfx_clear_cache(void)
{
    rsfx_sound_t* sound = g_sound_cache;
    while (sound)
    {
        rsfx_sound_t* next = sound->next;
        rsfx_free_sound(sound->handle);
        sound = next;
    }
    g_sound_cache = NULL;
}

void rsfx_terminate(void)
{
    rsfx_clear_cache();
    g_sfx_initialized        = 0;
    g_sound_pool_initialized = false;
    g_sound_free_top         = 0;
}

bool rsfx_play_sound(rsfx_sound_handle handle, bool loop)
{
    rsfx_sound_t* sound = rsfx_sound_from_handle(handle);
    if (!sound || !sound->loaded)
        return false;
    sound->loop = loop ? 1 : 0;
    ma_decoder_seek_to_pcm_frame(&sound->decoder, 0);
    ma_device_stop(&sound->device); // Ensure stopped before starting
    return ma_device_start(&sound->device) == MA_SUCCESS;
}

void rsfx_stop_sound(rsfx_sound_handle handle)
{
    rsfx_sound_t* sound = rsfx_sound_from_handle(handle);
    if (!sound || !sound->loaded)
        return;
    ma_device_stop(&sound->device);
}

void rsfx_set_volume(rsfx_sound_handle handle, float volume)
{
    rsfx_sound_t* sound = rsfx_sound_from_handle(handle);
    if (!sound || !sound->loaded)
        return;
    ma_device_set_master_volume(&sound->device, volume);
}
