#version 460

#define MAX_LIGHTS 3

layout(location = 0) in vec3 aPos;       // Vertex position
layout(location = 1) in vec3 aNormal;    // Vertex normal
layout(location = 2) in vec2 aTexCoord;  // Texture coordinates

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

uniform vec3 CameraPos;        // Camera position in world space
uniform vec3 LightPosition[MAX_LIGHTS]; // Light positions in world space

out vec3 LightDir[MAX_LIGHTS];  // Light directions in tangent space
out vec3 ViewDir;               // View direction in tangent space
out vec2 TexCoord;              // Texture coordinates
out float FragDistance;         // Distance from the camera to the fragment

void main()
{
    // Transform vertex position to world space
    vec3 FragPos = vec3(ModelMatrix * vec4(aPos, 1.0));

    // Calculate light directions for all lights
    for (int i = 0; i < MAX_LIGHTS; i++) {
        LightDir[i] = LightPosition[i] - FragPos;
    }

    // Calculate view direction in world space
    ViewDir = CameraPos - FragPos; // Corrected: Use CameraPos directly

    // Pass texture coordinates
    TexCoord = aTexCoord;

    // Calculate the distance from the camera to the fragment
    FragDistance = length(ViewDir);

    // Final vertex position
    gl_Position = ProjectionMatrix * ViewMatrix * ModelMatrix * vec4(aPos, 1.0);
}