#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raster_math.h"

typedef struct rtransform {
    vec3 position;
    vec3 scale;
    quat rotation;
    mat4x4 local;
    mat4x4 world;
    struct rtransform* parent;
} rtransform_t;

rtransform_t* rtransform_create(void);
void rtransform_destroy(rtransform_t* transform);
void rtransform_set_parent(rtransform_t* transform, rtransform_t* parent);
void rtransform_set_position(rtransform_t* transform, vec3 position);
void rtransform_set_scale(rtransform_t* transform, vec3 scale);
void rtransform_set_rotation_axis_angle(rtransform_t* transform, vec3 axis, float angle);
void rtransform_get_world_position(rtransform_t* transform, vec3 out_position);
void rtransform_update(rtransform_t* transform);

#ifdef __cplusplus
}
#endif