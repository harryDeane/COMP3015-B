#version 460

in vec3 gNormal;
in vec2 gTexCoord;
in vec3 gPosition;

uniform sampler2D mossTexture;
uniform sampler2D brickTexture;
uniform float ExplosionFactor;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(mossTexture, gTexCoord);
    
    // Make fragments more transparent as explosion progresses
    texColor.a *= 1.0 - ExplosionFactor;
    
    // Add glow effect
    vec3 glow = vec3(1.0, 0.5, 0.2) * ExplosionFactor * 2.0;
    
    FragColor = texColor + vec4(glow, 0.0);
}