out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform bool uUseTexture;
uniform float uTime;

void main()
{
    vec4 finalColor;
    float alpha;
    
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, TexCoord);
        if (texColor.a < 0.01) {
            discard; // Skip fully transparent pixels
        }
        finalColor = vec4(texColor.rgb * uColor, texColor.a);
        alpha = texColor.a;
    } else {
        finalColor = vec4(uColor, 1.0);
        alpha = 1.0;
    }

    // Only write to depth buffer for mostly opaque pixels
    if (alpha < 0.9) {
        gl_FragDepth = 1.0; // Far depth for transparent pixels
    } else {
        gl_FragDepth = gl_FragCoord.z;
    }
    
    FragColor = finalColor;
}