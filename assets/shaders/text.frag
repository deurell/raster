#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uColor;

void main() {
    // Sample from texture (red channel contains our glyph)
    float textAlpha = texture(uTexture, TexCoord).r;
    
    // Apply text color with alpha from texture
    // Add a threshold to make the text sharper
    if (textAlpha > 0.1) {
        textAlpha = smoothstep(0.1, 0.5, textAlpha);
    }
    
    FragColor = vec4(uColor, textAlpha);
}