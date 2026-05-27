#version 330 core

in vec3 fragScaledModelPos;

out vec4 outColor;

void main()
{
    vec3 i = floor(fragScaledModelPos);
    float white = mod(i.x + i.y + i.z, 2.);
    outColor = vec4(vec3(0.10) + white * vec3(0.65), 1.0);
}