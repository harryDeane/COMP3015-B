#version 460

in float Transp;
in vec2 TexCoord;

uniform sampler2D ParticleTex;

out vec4 FragColor;

void main()
{
    FragColor = texture(ParticleTex, TexCoord);
    FragColor.a *= Transp;
    
    if (FragColor.a < 0.01)
        discard;
}