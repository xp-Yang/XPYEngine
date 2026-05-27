#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    mat4 projectionView;
    vec3 fragWorldPos;          //世界坐标
    vec3 fragWorldNormal;       //世界坐标
    vec2 fragUV;
    vec4 fragPosLightSpace;     //裁剪坐标
} gs_in[];

void main()
{
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    EndPrimitive();

    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();

    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    EndPrimitive();
}