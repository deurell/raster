out vec4 FragColor;
in  vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec3      uColor;
uniform bool      uUseTexture;
uniform float     uTime;

void main()
{
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, TexCoord);
        FragColor = vec4(texColor.rgb * uColor, texColor.a);
    } else {
        FragColor = vec4(uColor, 1.0);
    }
}