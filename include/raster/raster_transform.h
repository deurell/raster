#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "raster_math.h"

typedef struct rtransform rtransform_t;

struct rtransform {
    vec3 position;
    vec3 scale;
    float rotation;    // 2D rotation around Z axis for simplicity
    mat4x4 local;     // Local transform matrix
    mat4x4 world;     // World transform matrix
    rtransform_t* parent;
    void* owner;      // Pointer to the sprite/text that owns this transform
};

rtransform_t* rtransform_create(void* owner);
void rtransform_destroy(rtransform_t* transform);
void rtransform_set_parent(rtransform_t* transform, rtransform_t* parent);
void rtransform_set_position(rtransform_t* transform, vec3 position);
void rtransform_set_scale(rtransform_t* transform, vec3 scale);
void rtransform_set_rotation(rtransform_t* transform, float rotation);
void rtransform_get_world_position(rtransform_t* transform, vec3 out_position);
void rtransform_update(rtransform_t* transform);

#ifdef __cplusplus
}
#endif