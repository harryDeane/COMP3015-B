#version 460

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vNormal[];
in vec2 vTexCoord[];
in vec3 vPosition[];

out vec3 gNormal;
out vec2 gTexCoord;
out vec3 gPosition;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform float ExplosionFactor;

void main() {
    vec3 center = (vPosition[0] + vPosition[1] + vPosition[2]) / 3.0;
    vec3 normal = normalize(vNormal[0] + vNormal[1] + vNormal[2]);
    
    for(int i = 0; i < 3; i++) {
        vec3 pos = vPosition[i];
        
        // Explode outward along normal
        pos += normal * ExplosionFactor * 5.0;
        
        // Add some randomness
        pos += vec3(sin(ExplosionFactor * 10.0 + i),
                   cos(ExplosionFactor * 8.0 + i),
                   sin(ExplosionFactor * 12.0 + i)) * ExplosionFactor * 2.0;
        
        gl_Position = ProjectionMatrix * ViewMatrix * vec4(pos, 1.0);
        gNormal = vNormal[i];
        gTexCoord = vTexCoord[i];
        gPosition = pos;
        EmitVertex();
    }
    EndPrimitive();
}