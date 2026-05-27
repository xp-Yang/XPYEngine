#version 330 core

out vec2 fragUV;

const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    vec2 p = positions[gl_VertexID];
    fragUV = p * 0.5 + 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
}
