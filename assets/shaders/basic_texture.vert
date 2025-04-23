layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec2 uPosition;
uniform vec2 uSize;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vec2 pos = aPos * uSize + uPosition;
    gl_Position = uProjection * uView * vec4(pos, 0.0, 1.0);
    TexCoord = aTexCoord;
}