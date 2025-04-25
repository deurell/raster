in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uColor;

void main() {
    float textAlpha = texture(uTexture, TexCoord).r;
    if (textAlpha < 0.01) {
        discard;
    } else if (textAlpha > 0.1) {
        textAlpha = smoothstep(0.1, 0.5, textAlpha);
    }
    
    FragColor = vec4(uColor, textAlpha);
}