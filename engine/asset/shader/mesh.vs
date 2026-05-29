#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_uv;
layout (location = 3) in ivec4 bone_ids;
layout (location = 4) in vec4 bone_weights;
layout (location = 5) in vec4 vertex_tangent;

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 bones[MAX_BONES];
uniform bool useSkinning;
uniform int bone_count;

out VS_OUT {
    vec3 fragWorldPos;          //????????
    vec3 fragWorldNormal;       //????????
    vec2 fragUV;
} vs_out;

// ??????????????????????mesh.vs ????? fs ?????????? VS_OUT ????????????
// ?? PBR ?? (gBuffer_pbr.fs / fowardLighting_pbr.fs) ??????? in??????????
out vec4 vWorldTangent; // xyz = ????????, w = ????

void main()
{
    vec4 skinned_pos = vec4(vertex_pos, 1.0);
    vec3 skinned_normal = vertex_normal;
    vec3 skinned_tangent = vertex_tangent.xyz;
    if (useSkinning && bone_count > 0) {
        mat4 skin_matrix = mat4(0.0);
        for (int i = 0; i < 4; ++i) {
            int bone_id = bone_ids[i];
            float weight = bone_weights[i];
            if (weight > 0.0 && bone_id >= 0 && bone_id < bone_count && bone_id < MAX_BONES) {
                skin_matrix += bones[bone_id] * weight;
            }
        }
        if (skin_matrix[3][3] == 0.0) {
            skin_matrix = mat4(1.0);
        }
        skinned_pos = skin_matrix * vec4(vertex_pos, 1.0);
        skinned_normal = mat3(skin_matrix) * vertex_normal;
        skinned_tangent = mat3(skin_matrix) * vertex_tangent.xyz;
    }

    vs_out.fragWorldPos = vec3(model * skinned_pos);
    vs_out.fragWorldNormal = normalize(mat3(model) * skinned_normal);
	vs_out.fragUV = vertex_uv;
    vWorldTangent = vec4(mat3(model) * skinned_tangent, vertex_tangent.w);
    //?????????gl_Position?Clip Space??????gs?????Clip Space????fs??????????? (??????) => NDC => (?????) => Screen Space?????????????Screen Space
    //????????
    gl_Position = projection * view * model * skinned_pos;
}