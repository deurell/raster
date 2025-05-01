in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uColor;

void main() {
    float textAlpha = texture(uTexture, TexCoord).r;
    if (textAlpha < 0.01) {
        discard; // This prevents writing to both color and depth buffer
    }
    
    // Opaque text for "RASTER"
    FragColor = vec4(uColor, 1.0);
    gl_FragDepth = gl_FragCoord.z; // Write to depth buffer for opaque pixels
}