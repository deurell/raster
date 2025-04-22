#include "raster/raster.h"
#include <stdio.h>

// Game state
typedef struct
{
    rgfx_sprite_t* red_sprite;
    rgfx_sprite_t* green_sprite;
    float          time;
    float          bounce_speed;
    float          orbit_speed;
} game_state_t;

static game_state_t G;

// Update callback
void game_update(float dt)
{
    G.time += dt;

    // Update sprite positions for animation
    rmath_vec3_t red_pos = { 0.0f, 0.3f * sin(G.time * G.bounce_speed), 0.0f };
    rgfx_sprite_set_position_vec3(G.red_sprite, red_pos);

    // Use a vector-based position setter for the green sprite
    rmath_vec2_t orbit_vec2 = { 0.5f * cos(G.time * G.orbit_speed), 0.5f * sin(G.time * G.orbit_speed) };
    rmath_vec3_t orbit_pos  = { orbit_vec2.x, orbit_vec2.y, 0.0f };
    rgfx_sprite_set_position_vec3(G.green_sprite, orbit_pos);

    // Check for input using the shorter prefix
    if (rinput_key_pressed(RINPUT_KEY_ESCAPE))
    {
        rapp_quit();
    }
}

// Draw callback
void game_draw(void)
{
    // Clear with a color struct
    rmath_color_t bg_color = { 0.13f, 0.56f, 0.88f };
    rgfx_clear_color(bg_color);

    rgfx_sprite_draw(G.red_sprite);
    rgfx_sprite_draw(G.green_sprite);
}

// Cleanup callback
void game_cleanup(void)
{
    rgfx_sprite_destroy(G.red_sprite);
    rgfx_sprite_destroy(G.green_sprite);
}

int main(void)
{
    // Initialize using descriptor-based approach
    rapp_desc_t app_desc = { .window     = { .title = "Raster Engine Demo", .width = 800, .height = 600 },
                             .update_fn  = game_update,
                             .draw_fn    = game_draw,
                             .cleanup_fn = game_cleanup };

    if (!rapp_init(&app_desc))
    {
        rlog_error("Failed to initialize the raster engine");
        return -1;
    }

    rgfx_sprite_desc_t sprite_desc = { .position             = { 0.0f, 0.0f, 0.0f },
                                       .size                 = { 0.5f, 0.5f },
                                       .color                = { 1.0f, 1.0f, 1.0f },
                                       .vertex_shader_path   = "assets/shaders/basic_texture.vert",
                                       .fragment_shader_path = "assets/shaders/basic_texture.frag",
                                       .texture_path         = "assets/textures/googly-e.png" };

    G.red_sprite = rgfx_sprite_create(&sprite_desc);

    sprite_desc.position     = (rmath_vec3_t){ 0.5f, 0.0f, 0.0f };
    sprite_desc.size         = (rmath_vec2_t){ 0.2f, 0.2f };
    sprite_desc.color        = (rmath_color_t){ 1.0f, 1.0f, 1.0f };
    sprite_desc.texture_path = "assets/textures/googly-b.png";
    G.green_sprite           = rgfx_sprite_create(&sprite_desc);

    // Initialize game state
    G.time         = 0.0f;
    G.bounce_speed = 2.2f;
    G.orbit_speed  = 1.2f;

    // Run the app (handles the main loop internally)
    rapp_run();

    return 0;
}
