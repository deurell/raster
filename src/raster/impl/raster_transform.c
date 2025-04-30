#include "raster/raster_transform.h"
#include <stdlib.h>

rtransform_t* rtransform_create(void) {
    rtransform_t* transform = (rtransform_t*)malloc(sizeof(rtransform_t));
    if (!transform) return NULL;

    transform->position[0] = 0.0f;
    transform->position[1] = 0.0f;
    transform->position[2] = 0.0f;

    transform->scale[0] = 1.0f;
    transform->scale[1] = 1.0f;
    transform->scale[2] = 1.0f;

    // Initialize quaternion to identity rotation
    quat_identity(transform->rotation);
    transform->parent = NULL;

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

void rtransform_set_rotation_axis_angle(rtransform_t* transform, vec3 axis, float angle) {
    if (transform) {
        quat_rotate(transform->rotation, angle, axis);
        rtransform_update(transform);
    }
}

void rtransform_set_rotation_quat(rtransform_t* transform, quat rotation) {
    if (transform) {
        memcpy(transform->rotation, rotation, sizeof(quat));
        rtransform_update(transform);
    }
}

void rtransform_get_world_position(rtransform_t* transform, vec3 out_position) {
    if (transform) {
        out_position[0] = transform->world[3][0];
        out_position[1] = transform->world[3][1];
        out_position[2] = transform->world[3][2];
    } else if (out_position) {
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
    
    // Rotation matrix from quaternion
    mat4x4_from_quat(rotation, transform->rotation);
    
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