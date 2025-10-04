/**
 * @file raster_sfx.h
 * @brief Basic audio functionality for the Raster engine
 */

#ifndef RASTER_SFX_H
#define RASTER_SFX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Sound handle type
     */
    typedef uint32_t rsfx_sound_handle;

#define RSFX_INVALID_SOUND_HANDLE 0u
#define RSFX_INVALID_SOUND        RSFX_INVALID_SOUND_HANDLE

    /**
     * @brief Initialize the audio system
     * @return true if initialization was successful, false otherwise
     */
    bool rsfx_init(void);

    /**
     * @brief Terminate the audio system
     */
    void rsfx_terminate(void);

    /**
     * @brief Load a sound from a file
     * @param path Path to the sound file
     * @return Handle to the loaded sound, or NULL if loading failed
     */
    rsfx_sound_handle rsfx_load_sound(const char* path);

    /**
     * @brief Free a loaded sound
     * @param sound Handle to the sound to free
     */
    void rsfx_free_sound(rsfx_sound_handle sound);

    /**
     * @brief Play a sound
     * @param sound Handle to the sound to play
     * @param loop Whether to loop the sound
     * @return true if the sound was played successfully, false otherwise
     */
    bool rsfx_play_sound(rsfx_sound_handle sound, bool loop);

    /**
     * @brief Stop a playing sound
     * @param sound Handle to the sound to stop
     */
    void rsfx_stop_sound(rsfx_sound_handle sound);

    /**
     * @brief Set the volume of a sound
     * @param sound Handle to the sound
     * @param volume Volume (0.0 to 1.0)
     */
    void rsfx_set_volume(rsfx_sound_handle sound, float volume);

#ifdef __cplusplus
}
#endif

#endif /* RASTER_SFX_H */
