#include "raster/raster_app.h"
#include "raster/raster_gfx.h"
#include "raster/raster_input.h"
#include "raster/raster_log.h"
#include <math.h>

int main(void)
{
    // Initialize the raster engine
    raster_app_config_t config = {.title = "Raster Engine Demo", .width = 800, .height = 600};

    if (!raster_app_init(&config))
    {
        raster_log_error("Failed to initialize the raster engine");
        return -1;
    }

    // Create sprites
    raster_sprite_t* red_sprite   = raster_sprite_create(0.0f, 0.0f, 0.3f, 0.3f, 1.0f, 0.0f, 0.0f);
    raster_sprite_t* green_sprite = raster_sprite_create(0.5f, 0.0f, 0.2f, 0.2f, 0.0f, 1.0f, 0.0f);

    float time         = 0.0f;
    float bounce_speed = 2.2f;
    float orbit_speed  = 1.2f;

    // Main game loop
    while (!raster_app_should_close())
    {
        // Poll events and get delta time
        raster_app_poll_events();
        float dt = raster_app_get_delta_time();
        time += dt;

        // Update sprite positions for animation
        raster_sprite_set_position(red_sprite, 0.0f, 0.3f * sin(time * bounce_speed));
        raster_sprite_set_position(green_sprite, 0.5f * cos(time * orbit_speed), 0.5f * sin(time * orbit_speed));

        // Check for input
        if (raster_input_key_pressed(RASTER_KEY_ESCAPE))
        {
            break;
        }

        // Render
        raster_gfx_clear(0.13f, 0.56f, 0.88f); // C64 blue background
        raster_sprite_draw(red_sprite);
        raster_sprite_draw(green_sprite);
        raster_app_present();
    }

    // Clean up
    raster_sprite_destroy(red_sprite);
    raster_sprite_destroy(green_sprite);
    raster_app_shutdown();

    return 0;
}
