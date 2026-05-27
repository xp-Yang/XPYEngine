#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 fragWorldPos;          //世界坐标
    vec3 fragWorldNormal;       //世界坐标
    vec2 fragUV;
} gs_in[];

out GS_OUT{
    vec3 fragWorldPos;          //世界坐标
    vec3 fragWorldNormal;       //世界坐标
    vec2 fragUV;
} gs_out;

uniform mat4 projectionView;

uniform float explosionRatio;

vec4 explode(vec3 position, vec3 dir)
{
    vec3 displacement = dir * explosionRatio; 
    return vec4(position + displacement, 1.0);
}

vec3 GetNormal()
{
   vec3 a = vec3(gs_in[0].fragWorldPos) - vec3(gs_in[1].fragWorldPos);
   vec3 b = vec3(gs_in[2].fragWorldPos) - vec3(gs_in[1].fragWorldPos);
   return normalize(cross(b, a));
}

void main() {    
    //vec3 normal = GetNormal();
    vec3 normal = (gs_in[0].fragWorldNormal + gs_in[1].fragWorldNormal + gs_in[2].fragWorldNormal) / 3;

    //输入的gs_in[0].pass_pos为世界坐标
    //输入和输出的gl_Position为裁剪空间坐标
    gl_Position = projectionView * explode(gs_in[0].fragWorldPos, normal);
    gs_out.fragUV = gs_in[0].fragUV;
    gs_out.fragWorldNormal = gs_in[0].fragWorldNormal;
    gs_out.fragWorldPos = gs_in[0].fragWorldPos;
    EmitVertex();
    gl_Position = projectionView * explode(gs_in[1].fragWorldPos, normal);
    gs_out.fragUV = gs_in[1].fragUV;
    gs_out.fragWorldNormal = gs_in[1].fragWorldNormal;
    gs_out.fragWorldPos = gs_in[1].fragWorldPos;
    EmitVertex();
    gl_Position = projectionView * explode(gs_in[2].fragWorldPos, normal);
    gs_out.fragUV = gs_in[2].fragUV;
    gs_out.fragWorldNormal = gs_in[2].fragWorldNormal;
    gs_out.fragWorldPos = gs_in[2].fragWorldPos;
    EmitVertex();
    EndPrimitive();
}