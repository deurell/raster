out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform bool uUseTexture;
uniform float uTime;
uniform float uFrequency;
uniform float uAmplitude;

void main()
{
    float yPos = TexCoord.y;
    float intensity = 1.0 - abs(2.0 * yPos - 1.0) * 0.7;
    intensity = pow(intensity, 0.75);
    intensity *= 1.0 + uAmplitude * sin(TexCoord.x * uFrequency + uTime * 2.0);
    
    vec3 barColor = vec3(intensity);
    vec4 finalColor;
    
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, TexCoord);
        if (texColor.a < 0.01) {
            discard;
        }
        finalColor = texColor * vec4(barColor * uColor, 1.0);
    } else {
        finalColor = vec4(barColor * uColor, 1.0);
    }

    // Always write to depth buffer since rasterbar is the background
    gl_FragDepth = gl_FragCoord.z;
    FragColor = finalColor;
}