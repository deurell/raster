#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform bool uUseTexture;

void main()
{
    if (uUseTexture) {
        FragColor = texture(uTexture, TexCoord) * vec4(uColor, 1.0);
    } else {
        FragColor = vec4(uColor, 1.0);
    }
}
