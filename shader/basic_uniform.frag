#version 460

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout (binding = 0) uniform sampler2D Tex1;
layout (binding = 1) uniform sampler2D Tex2;
layout (binding = 2) uniform sampler2D RenderTex;

layout (location = 0) out vec4 FragColour;

uniform float edgeThreshold;
uniform int pass;
uniform bool IsPlane; // New uniform to check if this is the plane
uniform vec3 PlaneColor; // New uniform for the plane's color

const vec3 lum = vec3(0.2126, 0.7152, 0.0722);

uniform struct LightInfo {
    vec4 Position;
    vec3 La;
    vec3 L;
    vec3 Direction;
    float Exponent;
    float Cutoff;
} Light;

uniform struct MaterialInfo {
    vec3 Kd;
    vec3 Ka;
    vec3 Ks;
    float Shininess;
} Material;

const int levels = 3;
const float scaleFactor = 1.0 / levels;

uniform struct FogInfo {
    float MaxDist;
    float MinDist;
    vec3 Colour;
} Fog;

float luminance(vec3 colour) {
    return dot(lum, colour);
}

vec3 Blinnphong(vec3 position, vec3 n) {
    vec3 diffuse = vec3(0), spec = vec3(0);

    vec3 texColour;
    if (IsPlane) {
        // Use the plane's color instead of sampling textures
        texColour = PlaneColor;
    } else {
        // Sample textures for non-plane objects
        vec4 pengTexColour = texture(Tex1, TexCoord);
        vec4 dirtTexColour = texture(Tex2, TexCoord);
        texColour = mix(pengTexColour.rgb, dirtTexColour.rgb, dirtTexColour.a);
    }

    vec3 ambient = Light.La * texColour;

    vec3 s = normalize(Light.Position.xyz - position);

    float cosAng = dot(-s, normalize(Light.Direction));
    float angle = acos(cosAng);
    float spotScale;

    if (angle >= 0.0 && angle < Light.Cutoff) {
        spotScale = pow(cosAng, Light.Exponent);
        float sDotN = max(dot(s, n), 0.0);
        diffuse = texColour * floor(sDotN * levels) * scaleFactor;

        if (sDotN > 0.00) {
            vec3 v = normalize(-position.xyz);
            vec3 h = normalize(v + s);
            spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess);
        }
    }

    return ambient + spotScale * (diffuse + spec) * Light.L;
}

vec4 pass1() {
    return vec4(Blinnphong(Position, normalize(Normal)), 1.0);
}

vec4 pass2() {
    ivec2 pix = ivec2(gl_FragCoord.xy);
    float s00 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(-1, 1)).rgb);
    float s10 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(-1, 0)).rgb);
    float s20 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(-1, -1)).rgb);
    float s01 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(0, 1)).rgb);
    float s21 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(0, -1)).rgb);
    float s02 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(1, 1)).rgb);
    float s12 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(1, 0)).rgb);
    float s22 = luminance(texelFetchOffset(RenderTex, pix, 0, ivec2(1, -1)).rgb);

    float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
    float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);
    float g = sx * sx + sy * sy;

    if (g > edgeThreshold)
        return vec4(1.0);
    else
        return texelFetch(RenderTex, pix, 0);
}

void main() {
    if (pass == 1) FragColour = pass1();
    if (pass == 2) FragColour = pass2();
}