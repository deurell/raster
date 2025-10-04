#include "raster/raster.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct
{
    rgfx_sprite_handle sprite_one;
    rgfx_sprite_handle sprite_two;
    rgfx_sprite_handle sprite_rasterbar;
    rgfx_text_handle   text;
    float          time;
    float          bounce_speed;
    float          orbit_speed;
} game_state_t;

static game_state_t G;

void game_update(float dt)
{
    G.time += dt;

    vec3 sprite_pos = { 0.0f, 0.8f * sinf(G.time * G.bounce_speed), 2.0f * sinf(G.time) };
    rgfx_sprite_set_position(G.sprite_one, sprite_pos);

    float orbit_radius = 1.5f;
    float theta        = G.time * G.orbit_speed;
    float phi          = 0.5f * sinf(G.time * 0.5f);

    vec3 local_pos = { orbit_radius * cosf(theta) * cosf(phi),
                       orbit_radius * sinf(phi),
                       orbit_radius * sinf(theta) * cosf(phi) };
    rgfx_sprite_set_position(G.sprite_two, local_pos);

    rtransform_t* transform = rgfx_sprite_get_transform(G.sprite_two);

    float angle = G.time * G.orbit_speed;
    quat  rotation;
    quat_rotate(rotation, angle, (vec3){ 0.0f, 0.0f, 1.0f });
    rtransform_set_rotation_quat(transform, rotation);

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
        rsfx_sound_handle sound = rsfx_load_sound("assets/sfx/bounce.wav");
        if (sound != RSFX_INVALID_SOUND_HANDLE)
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
    rgfx_sprite_draw(G.sprite_one);
    rgfx_sprite_draw(G.sprite_two);
    rgfx_text_draw(G.text);
}

void game_cleanup(void)
{
    rlog_info("Cleaning up game resources");
    if (G.sprite_one != RGFX_INVALID_SPRITE_HANDLE)
    {
        rgfx_sprite_destroy(G.sprite_one);
        G.sprite_one = RGFX_INVALID_SPRITE_HANDLE;
    }
    if (G.sprite_two != RGFX_INVALID_SPRITE_HANDLE)
    {
        rgfx_sprite_destroy(G.sprite_two);
        G.sprite_two = RGFX_INVALID_SPRITE_HANDLE;
    }
    if (G.sprite_rasterbar != RGFX_INVALID_SPRITE_HANDLE)
    {
        rgfx_sprite_destroy(G.sprite_rasterbar);
        G.sprite_rasterbar = RGFX_INVALID_SPRITE_HANDLE;
    }
    if (G.text != RGFX_INVALID_TEXT_HANDLE)
    {
        rgfx_text_destroy(G.text);
        G.text = RGFX_INVALID_TEXT_HANDLE;
    }
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
        rlog_info("Audio system initialized successfully");
        rsfx_sound_handle bgm = rsfx_load_sound("assets/sfx/background.mp3");
        if (bgm != RSFX_INVALID_SOUND_HANDLE)
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
                                       .scale                = { 1.0f, 1.0f, 1.0f },
                                       .color                = { 1.0f, 1.0f, 1.0f },
                                       .vertex_shader_path   = "assets/shaders/basic_texture.vert",
                                       .fragment_shader_path = "assets/shaders/basic_texture.frag",
                                       .texture_path         = "assets/textures/googly-a.png" };

    G.sprite_one = rgfx_sprite_create(&sprite_desc);
    if (G.sprite_one == RGFX_INVALID_SPRITE_HANDLE)
    {
        rlog_fatal("Failed to create sprite_one");
        return -1;
    }

    rgfx_sprite_desc_t sprite_two_desc = { .position             = { 1.5f, 0.0f, 0.0f },
                                           .scale                = { 0.5f, 0.5f, 0.5f },
                                           .color                = { 1.0f, 1.0f, 1.0f },
                                           .vertex_shader_path   = "assets/shaders/basic_texture.vert",
                                           .fragment_shader_path = "assets/shaders/basic_texture.frag",
                                           .texture_path         = "assets/textures/googly-e.png" };

    G.sprite_two = rgfx_sprite_create(&sprite_two_desc);
    if (G.sprite_two == RGFX_INVALID_SPRITE_HANDLE)
    {
        rlog_fatal("Failed to create sprite_two");
        return -1;
    }

    rgfx_sprite_set_parent(G.sprite_two, G.sprite_one);

    rgfx_sprite_desc_t rasterbar_desc = {
        .position             = { 0.0f, 0.0f, 0.0f },
        .scale                = { 100.0f, 0.5f, 1.0f },
        .color                = { 1.0f, 1.0f, 1.0f },
        .vertex_shader_path   = "assets/shaders/rasterbar.vert",
        .fragment_shader_path = "assets/shaders/rasterbar.frag",
        .uniforms             = { { .name = "uFrequency", .type = RGFX_UNIFORM_FLOAT, .uniform_float = 5.0f },
                                  { .name = "uAmplitude", .type = RGFX_UNIFORM_FLOAT, .uniform_float = 0.5f } }
    };

    G.sprite_rasterbar = rgfx_sprite_create(&rasterbar_desc);
    if (G.sprite_rasterbar == RGFX_INVALID_SPRITE_HANDLE)
    {
        rlog_fatal("Failed to create rasterbar sprite");
        return -1;
    }

    rgfx_text_desc_t text_desc = { .font_path    = "assets/fonts/roboto.ttf",
                                   .font_size    = 64.0f,
                                   .text         = "RASTER\nENGINE\nDEMO",
                                   .position     = { 0.0f, 0.0f, 0.0f },
                                   .text_color   = { 1.0f, 1.0f, 1.0f },
                                   .line_spacing = 0.9f,
                                   .alignment    = RGFX_TEXT_ALIGN_CENTER };

    G.text = rgfx_text_create(&text_desc);
    if (G.text == RGFX_INVALID_TEXT_HANDLE)
    {
        rlog_fatal("Failed to create text object");
        return -1;
    }

    G.time         = 0.0f;
    G.bounce_speed = 2.2f;
    G.orbit_speed  = 1.2f;

    rapp_run();

    return 0;
}
