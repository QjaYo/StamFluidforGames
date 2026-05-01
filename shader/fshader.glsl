#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uDensTex;

void main()
{
    float den = texture(uDensTex, vTexCoord).r;
    FragColor = vec4(vec3(den), 1.0);
}
