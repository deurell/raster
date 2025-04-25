layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uPosition;
uniform vec2 uSize;

out vec2 TexCoord;

void main() {
    // Transform position - use a simpler approach
    vec3 pos = aPos;
    
    // Scale by size - make y negative to flip the texture right side up
    pos.x *= uSize.x;
    pos.y *= uSize.y;  // Removed the negative sign to fix the flip
    
    // Add position
    pos += uPosition;
    
    // Convert to clip space
    gl_Position = uProjection * uView * vec4(pos, 1.0);
    
    // Pass texture coordinates to fragment shader
    TexCoord = aTexCoord;
}