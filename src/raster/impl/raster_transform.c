#include "raster/raster_transform.h"
#include "raster/raster_gfx.h"
#include <stdlib.h>

rtransform_t* rtransform_create(void* owner) {
    rtransform_t* transform = (rtransform_t*)malloc(sizeof(rtransform_t));
    if (!transform) return NULL;

    // Initialize position
    transform->position[0] = 0.0f;
    transform->position[1] = 0.0f;
    transform->position[2] = 0.0f;

    // Initialize scale to 1
    transform->scale[0] = 1.0f;
    transform->scale[1] = 1.0f;
    transform->scale[2] = 1.0f;

    transform->rotation = 0.0f;
    transform->parent = NULL;
    transform->owner = owner;

    // Initialize matrices to identity
    mat4x4_identity(transform->local);
    mat4x4_identity(transform->world);

    return transform;
}

void rtransform_destroy(rtransform_t* transform) {
    if (transform) {
        free(transform);
    }
}

void rtransform_set_parent(rtransform_t* transform, rtransform_t* parent) {
    if (transform) {
        transform->parent = parent;
        rtransform_update(transform);
    }
}

void rtransform_set_position(rtransform_t* transform, vec3 position) {
    if (transform) {
        vec3_dup(transform->position, position);
        rtransform_update(transform);
    }
}

void rtransform_set_scale(rtransform_t* transform, vec3 scale) {
    if (transform) {
        vec3_dup(transform->scale, scale);
        rtransform_update(transform);
    }
}

void rtransform_set_rotation(rtransform_t* transform, float rotation) {
    if (transform) {
        transform->rotation = rotation;
        rtransform_update(transform);
    }
}

void rtransform_get_world_position(rtransform_t* transform, vec3 out_position) {
    if (transform) {
        // World position is in the last column of the world matrix
        out_position[0] = transform->world[3][0];
        out_position[1] = transform->world[3][1];
        out_position[2] = transform->world[3][2];
    } else {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
    }
}

void rtransform_update(rtransform_t* transform) {
    if (!transform) return;

    // Build local transform matrix
    mat4x4 translation, rotation, scale, temp;
    
    // Translation matrix
    mat4x4_identity(translation);
    translation[3][0] = transform->position[0];
    translation[3][1] = transform->position[1];
    translation[3][2] = transform->position[2];
    
    // Rotation matrix (2D around Z axis)
    mat4x4_identity(rotation);
    float c = cosf(transform->rotation);
    float s = sinf(transform->rotation);
    rotation[0][0] = c;
    rotation[0][1] = -s;
    rotation[1][0] = s;
    rotation[1][1] = c;
    
    // Scale matrix
    mat4x4_identity(scale);
    scale[0][0] = transform->scale[0];
    scale[1][1] = transform->scale[1];
    scale[2][2] = transform->scale[2];
    
    // Combine: local = translation * rotation * scale
    mat4x4_mul(temp, translation, rotation);
    mat4x4_mul(transform->local, temp, scale);
    
    // Calculate world matrix
    if (transform->parent) {
        mat4x4_mul(transform->world, transform->parent->world, transform->local);
    } else {
        mat4x4_dup(transform->world, transform->local);
    }
}

void rtransform_set_generic_parent(void* child, void* parent) {
    rtransform_t* child_transform = rtransform_get(child);
    rtransform_t* parent_transform = parent ? rtransform_get(parent) : NULL;
    
    if (child_transform) {
        rtransform_set_parent(child_transform, parent_transform);
    }
}