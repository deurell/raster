#include "raster/raster.h" // Use the unified header

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
    rgfx_sprite_set_position(G.red_sprite, 0.0f, 0.3f * sin(G.time * G.bounce_speed));

    // Use a vector-based position setter for the green sprite
    rmath_vec2_t orbit_pos = { 0.5f * cos(G.time * G.orbit_speed), 0.5f * sin(G.time * G.orbit_speed) };
    rgfx_sprite_set_position_vec2(G.green_sprite, orbit_pos);

    rinput_debug_print_pressed_keys();
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

    // Draw sprites
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

    rgfx_sprite_desc_t sprite_desc = {
        .position = { 0.0f, 0.0f }, .size = { 0.3f, 0.3f }, .color = { 1.0f, 0.0f, 0.0f }, .z_order = 0
    };

    G.red_sprite = rgfx_sprite_create(&sprite_desc);

    // Modify the descriptor for the green sprite
    sprite_desc.position.x = 0.5f;
    sprite_desc.size       = (rmath_vec2_t){ 0.2f, 0.2f };
    sprite_desc.color      = (rmath_color_t){ 0.0f, 1.0f, 0.0f };
    G.green_sprite         = rgfx_sprite_create(&sprite_desc);

    // Initialize game state
    G.time         = 0.0f;
    G.bounce_speed = 2.2f;
    G.orbit_speed  = 1.2f;

    // Run the app (handles the main loop internally)
    rapp_run();

    return 0;
}
