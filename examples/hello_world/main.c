#include "raster/raster_app.h"
#include "raster/raster_gfx.h"
#include "raster/raster_input.h"
#include "raster/raster_log.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct
{
    rgfx_sprite_t* red_sprite;
    rgfx_sprite_t* green_sprite;
    rapp_desc_t app_desc;    // Store app descriptor
    float          time;
    float          bounce_speed;
    float          orbit_speed;
} game_state_t;

static game_state_t G;

void game_update(float dt)
{
    G.time += dt;

    // Example of using app_desc in update
    float window_width = G.app_desc.window.width;
    float window_height = G.app_desc.window.height;

    vec3 position = { 0.0f, 0.3f * sin(G.time * G.bounce_speed), 0.0f };
    rgfx_sprite_set_position(G.red_sprite, position);

    vec3 orbit_pos = { 0.5f * cos(G.time * G.orbit_speed), 0.5f * sin(G.time * G.orbit_speed), 0.0f };
    rgfx_sprite_set_position(G.green_sprite, orbit_pos);

    // move the camera
    rgfx_camera_t* camera = rapp_get_main_camera();
    if (camera)
    {
        vec3 camera_pos = { 0.0f, 0.0f, 5.0f + 2.0f * sin(G.time) };
        rgfx_camera_set_position(camera, camera_pos);
    }

    if (rinput_key_pressed(RINPUT_KEY_ESCAPE))
    {
        rapp_quit();
    }
}

void game_draw(void)
{
    color bg_color = { 0.13f, 0.56f, 0.88f };
    rgfx_clear_color(bg_color);

    rgfx_sprite_draw(G.red_sprite);
    rgfx_sprite_draw(G.green_sprite);
}

void game_cleanup(void)
{
    rlog_info("Cleaning up game resources");
    rgfx_sprite_destroy(G.red_sprite);
    rgfx_sprite_destroy(G.green_sprite);
}

int main(void)
{
    rapp_desc_t app_desc = { 
        .window = { 
            .title = "Raster Engine Demo", 
            .width = 800, 
            .height = 600 
        },
        .update_fn = game_update,
        .draw_fn = game_draw,
        .cleanup_fn = game_cleanup,
        .camera = {
            .position = { 0.0f, 0.0f, 5.0f },
            .target = { 0.0f, 0.0f, 0.0f },
            .up = { 0.0f, 1.0f, 0.0f },
            .fov = 45.0f * (3.14159f / 180.0f),
            .aspect = 800.0f / 600.0f,
            .near = 0.1f,
            .far = 100.0f
        }
    };

    // Store app_desc in game state
    G.app_desc = app_desc;

    if (!rapp_init(&app_desc))
    {
        rlog_error("Failed to initialize the raster engine");
        return -1;
    }
    rlog_info("Raster engine initialized successfully");

    // Create sprites
    rgfx_sprite_desc_t sprite_desc = { 
        .position = { 0.0f, 0.0f, 0.0f },
        .size = { 0.5f, 0.5f },
        .color = { 1.0f, 1.0f, 1.0f },
        .vertex_shader_path = "assets/shaders/basic_texture.vert",
        .fragment_shader_path = "assets/shaders/basic_texture.frag",
        .texture_path = "assets/textures/googly-e.png" 
    };

    G.red_sprite = rgfx_sprite_create(&sprite_desc);

    rgfx_sprite_desc_t green_sprite_desc = { 
        .position = { 0.5f, 0.0f, 0.0f },
        .size = { 0.4f, 0.4f },
        .color = { 1.0f, 1.0f, 1.0f },
        .vertex_shader_path = "assets/shaders/basic_texture.vert",
        .fragment_shader_path = "assets/shaders/basic_texture.frag",
        .texture_path = "assets/textures/googly-b.png" 
    };

    G.green_sprite = rgfx_sprite_create(&green_sprite_desc);

    G.time = 0.0f;
    G.bounce_speed = 2.2f;
    G.orbit_speed = 1.2f;

    rapp_run();

    return 0;
}
