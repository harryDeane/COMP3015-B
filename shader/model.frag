#version 460

#define MAX_LIGHTS 3 // Defines the number of lights

in vec3 LightDir[MAX_LIGHTS];  // Light directions in tangent space
in vec3 ViewDir;               // View direction in tangent space
in vec2 TexCoord;              // Texture coordinates

layout(binding = 0) uniform sampler2D ColorTex;     // Color texture
layout(binding = 1) uniform sampler2D NormalMapTex; // Normal map texture

layout(location = 0) out vec4 FragColor;

uniform bool IsPlane; // New uniform to check if this is the plane

// Light and material uniforms
uniform struct LightInfo {
    vec4 Position;   // Light position
    vec3 La;        // Ambient light intensity
    vec3 L;         // Diffuse and specular light intensity
    vec3 Direction; // Spotlight direction
    float Cutoff;   // Spotlight cutoff angle
} Lights[MAX_LIGHTS];

uniform struct MaterialInfo {
    vec3 Kd;            // Diffuse reflectivity
    vec3 Ka;            // Ambient reflectivity
    vec3 Ks;            // Specular reflectivity
    float Shininess;    // Specular shininess factor
} Material;

uniform struct FogInfo {
    vec3 Color;     // Fog color
    float MinDist;  // Minimum distance for fog to start
    float MaxDist;  // Maximum distance for fog to fully cover
} Fog;

// Blinn-Phong lighting calculation for a single light
vec3 blinnPhong(vec3 n, LightInfo light, vec3 lightDir) {
    vec3 diffuse = vec3(0), spec = vec3(0);

    // Sample the color texture
    vec3 texColor;
    if (IsPlane) {
        vec4 Tex1 = texture(ColorTex, TexCoord);
        vec4 Tex2 = texture(NormalMapTex, TexCoord);
        // Use the plane's color instead of sampling textures
        texColor = mix(Tex1.rgb, Tex2.rgb, Tex2.a);
    } 
    else {
    texColor = texture(ColorTex, TexCoord).rgb;
    }

    // Ambient component
    vec3 ambient = light.La * texColor;

    // Normalize the light direction
    vec3 s = normalize(lightDir);

    // Spotlight effect
    float theta = dot(normalize(light.Direction), -s);
    float epsilon = light.Cutoff - 0.05; // Soft edge
    float intensity = clamp((theta - epsilon) / 0.05, 0.0, 1.0);

    // Diffuse component
    float sDotN = max(dot(s, n), 0.0);
    diffuse = texColor * sDotN * intensity;

    // Specular component (Blinn-Phong)
    if (sDotN > 0.0) {
        vec3 v = normalize(ViewDir);
        vec3 h = normalize(v + s); // Halfway vector
        spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess) * intensity;
    }

    // Combine ambient, diffuse, and specular components
    return ambient + (diffuse + spec) * light.L;
}

// Calculate fog factor
float fogFactor(float dist) {
    return clamp((Fog.MaxDist - dist) / (Fog.MaxDist - Fog.MinDist), 0.0, 1.0);
}

void main() {
    // Sample the normal map and transform to tangent space
    vec3 norm = texture(NormalMapTex, TexCoord).xyz;
    norm.xy = 2.0 * norm.xy - 1.0; // Unpack from [0, 1] to [-1, 1]

    // Normalize the normal
    norm = normalize(norm);

    // Initialize the final color
    vec3 finalColor = vec3(0.0);

    // Accumulate the lighting contributions from all lights
    for (int i = 0; i < MAX_LIGHTS; i++) {
        finalColor += blinnPhong(norm, Lights[i], LightDir[i]);
    }

    // Calculate the distance from the camera to the fragment
    float dist = length(ViewDir);

    // Calculate the fog factor
    float fogFactor = fogFactor(dist);

    // Blend the final color with the fog color
    finalColor = mix(Fog.Color, finalColor, fogFactor);

    // Output the final color
    FragColor = vec4(finalColor, 1.0);
 }