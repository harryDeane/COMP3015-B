#version 430

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vPosition;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

void main() {
    vNormal = normalize(mat3(ModelMatrix) * normal);
    vTexCoord = texCoord;
    vPosition = vec3(ModelMatrix * vec4(position, 1.0));
    
    gl_Position = vec4(position, 1.0);
}