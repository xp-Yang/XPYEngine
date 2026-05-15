#version 330 core

in vec3 fragUV;

uniform samplerCube skybox;
layout(location = 0) out vec4 FragColor;

void main()
{
    gl_FragDepth = 0.9999;
    FragColor = texture(skybox, fragUV);
}
