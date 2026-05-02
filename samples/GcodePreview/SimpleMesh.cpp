#include "SimpleMesh.hpp"
#include "SimpleMesh.hpp"

std::shared_ptr<SimpleMesh> SimpleMesh::create_vertex_normal_cuboid_mesh(const std::array<Vec3, 2> &face_center_pos, const Vec3 &face_left_vec, const Vec3 &face_up_vec)
{
    std::vector<SimpleVertex> vertices;
    vertices.reserve(8);

    for (int i = 0; i < 2; i++)
    {
        const auto &center_pos = face_center_pos[i];
        const auto &left_vec = face_left_vec;
        const auto &up_vec = face_up_vec;

        Vec3 left_pos = center_pos + left_vec;
        Vec3 right_pos = center_pos - left_vec;
        Vec3 up_pos = center_pos + up_vec;
        Vec3 down_pos = center_pos - up_vec;

        Vec3 left_normal = Math::Normalize(left_vec);
        Vec3 right_normal = -left_normal;
        Vec3 up_normal = Math::Normalize(up_vec);
        Vec3 down_normal = -up_normal;

        vertices.push_back(SimpleVertex{left_pos, left_normal});
        vertices.push_back(SimpleVertex{down_pos, down_normal});
        vertices.push_back(SimpleVertex{right_pos, right_normal});
        vertices.push_back(SimpleVertex{up_pos, up_normal});
    }

    std::vector<int> indices = {
        0,
        1,
        2,
        0,
        2,
        3, // 前
        7,
        6,
        5,
        7,
        5,
        4, // 后
        4,
        5,
        1,
        4,
        1,
        0, // 左
        3,
        2,
        6,
        3,
        6,
        7, // 右
        1,
        5,
        6,
        1,
        6,
        2, // 下
        4,
        0,
        3,
        4,
        3,
        7, // 上
    };

    return std::make_shared<SimpleMesh>(vertices, indices);
}

std::shared_ptr<SimpleMesh> SimpleMesh::create_vertex_normal_arc_mesh(const std::vector<Vec3> &face_center_pos, const std::vector<Vec3> &face_left_vec, const std::vector<Vec3> &face_up_vec)
{
    int face_count = face_center_pos.size();
    assert(face_count == face_left_vec.size() && face_count == face_up_vec.size());
    assert(face_count >= 2);

    std::vector<SimpleVertex> vertices;
    vertices.reserve(4 * face_count);
    for (int i = 0; i < face_count; i++)
    {
        const auto &center_pos = face_center_pos[i];
        const auto &left_vec = face_left_vec[i];
        const auto &up_vec = face_up_vec[i];

        Vec3 left_pos = center_pos + left_vec;
        Vec3 right_pos = center_pos - left_vec;
        Vec3 up_pos = center_pos + up_vec;
        Vec3 down_pos = center_pos - up_vec;

        Vec3 left_normal = Math::Normalize(left_vec);
        Vec3 right_normal = -left_normal;
        Vec3 up_normal = Math::Normalize(up_vec);
        Vec3 down_normal = -up_normal;

        vertices.push_back(SimpleVertex{left_pos, left_normal});
        vertices.push_back(SimpleVertex{down_pos, down_normal});
        vertices.push_back(SimpleVertex{right_pos, right_normal});
        vertices.push_back(SimpleVertex{up_pos, up_normal});
    }

    int sub_segment_count = face_count - 1;
    std::vector<int> indices;
    indices.reserve(5 * 2 * 3 + 4 * (sub_segment_count - 2) * 3);
    std::vector<int> front_indices = {0, 1, 2, 0, 2, 3};
    indices.insert(indices.end(), front_indices.begin(), front_indices.end());
    std::vector<int> sub_indices = {
        4,
        5,
        1,
        4,
        1,
        0, // 左
        3,
        2,
        6,
        3,
        6,
        7, // 右
        1,
        5,
        6,
        1,
        6,
        2, // 下
        4,
        0,
        3,
        4,
        3,
        7, // 上
    };
    for (int i = 0; i < sub_segment_count; i++)
    {
        for (int sub_index : sub_indices)
        {
            indices.push_back(sub_index + i * 4);
        }
    }
    int vertex_count = vertices.size();
    std::vector<int> back_indices = {vertex_count - 1, vertex_count - 2, vertex_count - 3, vertex_count - 1, vertex_count - 3, vertex_count - 4};
    indices.insert(indices.end(), back_indices.begin(), back_indices.end());

    return std::make_shared<SimpleMesh>(vertices, indices);
}

std::shared_ptr<SimpleMesh> SimpleMesh::merge(const std::vector<std::shared_ptr<SimpleMesh>> &meshes)
{
    std::vector<SimpleVertex> vertices;
    std::vector<int> indices;
    vertices.reserve(8 * meshes.size());
    indices.reserve(36 * meshes.size());

    for (auto &mesh : meshes)
    {
        vertices.insert(vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
    }

    int indices_offset = 0;
    for (int i = 0; i < meshes.size(); i++)
    {
        std::vector<int> indices_i(meshes[i]->indices.size());
        std::transform(meshes[i]->indices.begin(), meshes[i]->indices.end(), indices_i.begin(), [indices_offset](const auto &index)
                       { return index + indices_offset; });
        indices.insert(indices.end(), indices_i.begin(), indices_i.end());
        indices_offset += meshes[i]->vertices.size();
    }

    return std::make_shared<SimpleMesh>(vertices, indices);
}