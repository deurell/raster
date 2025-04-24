layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec3 uPosition;
uniform vec2 uSize;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;

void main()
{
    vec3 pos3d = vec3(aPos * uSize, 0.0) + uPosition;
    gl_Position = uProjection * uView * vec4(pos3d, 1.0);
    TexCoord = aTexCoord;
}