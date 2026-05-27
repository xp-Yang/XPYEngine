#version 330 core
layout (location = 0) in vec3 vertex_pos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 modelScale;

out vec3 fragScaledModelPos;

void main()
{
    fragScaledModelPos = vec3(modelScale * vec4(vertex_pos, 1.0));
    gl_Position = projection * view * model * vec4(vertex_pos, 1.0);
}