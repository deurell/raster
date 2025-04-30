#include "raster/raster.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct
{
    rgfx_sprite_t* sprite_one;
    rgfx_sprite_t* sprite_two;
    rgfx_sprite_t* sprite_rasterbar;
    rgfx_text_t*   text;
    float          time;
    float          bounce_speed;
    float          orbit_speed;
} game_state_t;

static game_state_t G;

void game_update(float dt)
{
    G.time += dt;

    // Update parent sprite position (bouncing motion)
    vec3 sprite_pos = { 0.0f, 0.8f * sinf(G.time * G.bounce_speed), 2.0f * sinf(G.time) };
    rgfx_sprite_set_position(G.sprite_one, sprite_pos);

    // Update child sprite's local rotation to create orbit
    float orbit_angle = G.time * G.orbit_speed;
    vec3 local_pos = { 1.5f * cosf(orbit_angle), 1.5f * sinf(orbit_angle), 0.0f };
    rgfx_sprite_set_position(G.sprite_two, local_pos);

    rgfx_sprite_set_uniform_float(G.sprite_rasterbar, "uFrequency", 0.8f + 0.5f * sinf(G.time));
    rgfx_sprite_set_uniform_float(G.sprite_rasterbar, "uAmplitude", 0.2f + 0.1f * cosf(G.time * 0.5f));

    rgfx_camera_t* camera = rapp_get_main_camera();
    if (camera)
    {
        vec3 camera_pos = { 0.0f, 0.0f, 5.0f };
        rgfx_camera_set_position(camera, camera_pos);
    }

    unsigned int chars[32];
    int          num_chars = rinput_get_chars(chars, 32);
    for (int i = 0; i < num_chars; ++i)
    {
        rlog_info("Char input: U+%04X\n", chars[i]);
    }

    if (rinput_key_pressed(RINPUT_KEY_ESCAPE))
    {
        rapp_quit();
    }
    if (rinput_key_down(RINPUT_KEY_0))
    {
        rlog_info("Key 0 pressed");
        rsfx_sound_t sound = rsfx_load_sound("assets/sfx/bounce.wav");
        if (sound)
        {
            rsfx_play_sound(sound, false);
        }
        else
        {
            rlog_error("Failed to load sound\n");
        }
    }
}

void game_draw(void)
{
    color bg_color = { 0.0f, 0.53f, 0.94f };
    rgfx_clear_color(bg_color);
    rgfx_sprite_draw(G.sprite_rasterbar);
    rgfx_text_draw(G.text);
    rgfx_sprite_draw(G.sprite_one);
    rgfx_sprite_draw(G.sprite_two);
}

void game_cleanup(void)
{
    rlog_info("Cleaning up game resources");
    rgfx_sprite_destroy(G.sprite_one);
    rgfx_sprite_destroy(G.sprite_two);
    rgfx_sprite_destroy(G.sprite_rasterbar);
    rgfx_text_destroy(G.text);
}

int main(void)
{
    rapp_desc_t app_desc = { .window     = { .title = "Raster Engine Demo", .width = 800, .height = 600 },
                             .update_fn  = game_update,
                             .draw_fn    = game_draw,
                             .cleanup_fn = game_cleanup,
                             .camera     = { .position = { 0.0f, 0.0f, 5.0f },
                                             .target   = { 0.0f, 0.0f, 0.0f },
                                             .up       = { 0.0f, 1.0f, 0.0f },
                                             .fov      = deg_to_rad(90.0f),
                                             .aspect   = 800.0f / 600.0f,
                                             .near     = 0.1f,
                                             .far      = 100.0f } };

    if (!rapp_init(&app_desc))
    {
        rlog_error("Failed to initialize the raster engine");
        return -1;
    }
    rlog_info("Raster engine initialized successfully");

    // Initialize audio
    if (!rsfx_init())
    {
        rlog_error("Failed to initialize audio system\n");
    }
    else
    {
        rsfx_sound_t bgm = rsfx_load_sound("assets/sfx/background.mp3");
        if (bgm)
        {
            rsfx_play_sound(bgm, true); // Loop background music
        }
        else
        {
            rlog_error("Failed to load background music\n");
        }
    }

    // Create sprites
    rgfx_sprite_desc_t sprite_desc = { .position             = { 0.0f, 0.0f, 0.0f },
                                       .size                 = { 1.0f, 1.0f },
                                       .color                = { 1.0f, 1.0f, 1.0f },
                                       .vertex_shader_path   = "assets/shaders/basic_texture.vert",
                                       .fragment_shader_path = "assets/shaders/basic_texture.frag",
                                       .texture_path         = "assets/textures/googly-a.png" };

    G.sprite_one = rgfx_sprite_create(&sprite_desc);

    rgfx_sprite_desc_t sprite_two_desc = { .position             = { 1.5f, 0.0f, 0.0f },
                                             .size                 = { 0.5f, 0.5f }, // Make the child sprite smaller
                                             .color                = { 1.0f, 1.0f, 1.0f },
                                             .vertex_shader_path   = "assets/shaders/basic_texture.vert",
                                             .fragment_shader_path = "assets/shaders/basic_texture.frag",
                                             .texture_path         = "assets/textures/googly-e.png" };

    G.sprite_two = rgfx_sprite_create(&sprite_two_desc);
    
    // Set up parent-child relationship using generic system
    rgfx_set_parent(G.sprite_two, G.sprite_one);

    rgfx_sprite_desc_t rasterbar_desc = {
        .position             = { 0.0f, 0.0f, 0.0f },
        .size                 = { 100.0f, 0.5f },
        .color                = { 1.0f, 1.0f, 1.0f },
        .vertex_shader_path   = "assets/shaders/rasterbar.vert",
        .fragment_shader_path = "assets/shaders/rasterbar.frag",
        .uniforms             = { { .name = "uFrequency", .type = RGFX_UNIFORM_FLOAT, .float_val = 5.0f },
                                  { .name = "uAmplitude", .type = RGFX_UNIFORM_FLOAT, .float_val = 0.5f } }
    };

    G.sprite_rasterbar = rgfx_sprite_create(&rasterbar_desc);

    // Create text
    rgfx_text_desc_t text_desc = { .font_path    = "assets/fonts/roboto.ttf",
                                   .font_size    = 64.0f,
                                   .text         = "RASTER",
                                   .position     = { 0.0f, 0.0f, 0.0f },
                                   .text_color   = { 1.0f, 1.0f, 1.0f },
                                   .line_spacing = 1.2f,
                                   .alignment    = RGFX_TEXT_ALIGN_CENTER };

    G.text = rgfx_text_create(&text_desc);

    G.time         = 0.0f;
    G.bounce_speed = 2.2f;
    G.orbit_speed  = 1.2f;

    rapp_run();

    return 0;
}
