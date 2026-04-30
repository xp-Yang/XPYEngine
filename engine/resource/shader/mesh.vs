#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_uv;
layout (location = 3) in ivec4 bone_ids;
layout (location = 4) in vec4 bone_weights;

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 bones[MAX_BONES];
uniform bool useSkinning;
uniform int bone_count;

out VS_OUT {
    vec3 fragWorldPos;          //世界坐标
    vec3 fragWorldNormal;       //世界坐标
    vec2 fragUV;
} vs_out;

void main()
{
    vec4 skinned_pos = vec4(vertex_pos, 1.0);
    vec3 skinned_normal = vertex_normal;
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
    }

    vs_out.fragWorldPos = vec3(model * skinned_pos);
    vs_out.fragWorldNormal = normalize(mat3(model) * skinned_normal);
	vs_out.fragUV = vertex_uv;
    //这里输出的gl_Position为Clip Space，给到gs时还是Clip Space，到fs时已经自动做了 (透视除法) => NDC => (视口变换) => Screen Space，变成了屏幕空间Screen Space
    //裁剪坐标
    gl_Position = projection * view * model * skinned_pos;
}