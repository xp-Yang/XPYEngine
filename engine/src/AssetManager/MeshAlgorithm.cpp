#include "AssetManager/MeshAlgorithm.hpp"

namespace MeshAlgorithm {
    
std::shared_ptr<Mesh> create_cube_mesh()
{
    // float cubeVertices[] =
    //{
    //     // pos                  // normal              // uv
    //     -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,    // 0  前面 左上 (从前面看)
    //     -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,    // 1  前面 左下
    //      0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,    // 2  前面 右下
    //      0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,    // 3  前面 右上

    //     0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,    // 4  后面 左上 (从后面看)
    //     0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,    // 5  后面 左下
    //    -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,    // 6  后面 右下
    //    -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,    // 7  后面 右上

    //    -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    // 8  左面 左上 (从左面看)
    //    -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    // 9  左面 左下
    //    -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,    // 10 左面 右下
    //    -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    // 11 左面 右上

    //     0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    // 12 右面 左上 (从右面看)
    //     0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    // 13 右面 左下
    //     0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f,    // 14 右面 右下
    //     0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    // 15 右面 右上

    //    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,    // 16 下面 左上 (从下面看)
    //    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    // 17 下面 左下
    //     0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,    // 18 下面 右下
    //     0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 19 下面 右上

    //    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,    // 20 上面 左上 (从上面看)
    //    -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    // 21 上面 左下
    //     0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    // 22 上面 右下
    //     0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    // 23 上面 右上
    //};

    std::array<Vec3, 8> vertex_positions;
    vertex_positions[0] = Vec3(-0.5f, 0.5f, 0.5f);
    vertex_positions[1] = Vec3(-0.5f, -0.5f, 0.5f);
    vertex_positions[2] = Vec3(0.5f, -0.5f, 0.5f);
    vertex_positions[3] = Vec3(0.5f, 0.5f, 0.5f);
    vertex_positions[4] = Vec3(0.5f, 0.5f, -0.5f);
    vertex_positions[5] = Vec3(0.5f, -0.5f, -0.5f);
    vertex_positions[6] = Vec3(-0.5f, -0.5f, -0.5f);
    vertex_positions[7] = Vec3(-0.5f, 0.5f, -0.5f);
    return create_cuboid_mesh(vertex_positions);
}

std::shared_ptr<Mesh> create_cuboid_mesh(const std::array<Vec3, 8> vertex_positions)
{
    std::vector<Vertex> vertices(24);

    std::array<int, 24> vertex_position_indices = {
        0,
        1,
        2,
        3, // 前面
        4,
        5,
        6,
        7, // 后面
        7,
        6,
        1,
        0, // 左面
        3,
        2,
        5,
        4, // 右面
        1,
        6,
        5,
        2, // 下面
        7,
        0,
        3,
        4, // 上面
    };

    Vec3 front_normal = Math::Normalize(vertex_positions[0] - vertex_positions[7]);
    Vec3 back_normal = -front_normal;
    Vec3 left_normal = Math::Normalize(vertex_positions[0] - vertex_positions[3]);
    Vec3 right_normal = -left_normal;
    Vec3 bottom_normal = Math::Normalize(vertex_positions[1] - vertex_positions[0]);
    Vec3 top_normal = -bottom_normal;
    std::array<Vec3, 6> face_normals = {
        front_normal,
        back_normal,
        left_normal,
        right_normal,
        bottom_normal,
        top_normal,
    };

    for (int i = 0; i < vertices.size(); i += 4)
    {
        vertices[i + 0] = {vertex_positions[vertex_position_indices[i + 0]], face_normals[i / 4], {0.0f, 1.0f}};
        vertices[i + 1] = {vertex_positions[vertex_position_indices[i + 1]], face_normals[i / 4], {0.0f, 0.0f}};
        vertices[i + 2] = {vertex_positions[vertex_position_indices[i + 2]], face_normals[i / 4], {1.0f, 0.0f}};
        vertices[i + 3] = {vertex_positions[vertex_position_indices[i + 3]], face_normals[i / 4], {1.0f, 1.0f}};
    }

    std::vector<int> indices =
        {
            0,
            1,
            2,
            0,
            2,
            3, // 前
            4,
            5,
            6,
            4,
            6,
            7, // 后
            8,
            9,
            10,
            8,
            10,
            11, // 左
            12,
            13,
            14,
            12,
            14,
            15, // 右
            16,
            17,
            18,
            16,
            18,
            19, // 下
            20,
            21,
            22,
            20,
            22,
            23, // 上
        };

    return std::make_shared<Mesh>(vertices, indices);
}

static void create_tetrahedron(std::vector<Triangle> &triangles, Vec3 &center)
{
    float side_length = 1.0f;
    Vec3 a = Vec3(0, 0, 0);
    Vec3 b = Vec3(side_length, 0, 0);
    Vec3 c = Vec3(side_length / 2.0f, 0, -side_length * sqrt(3.0f) / 2.0f);
    Vec3 d = Vec3(side_length / 2.0f, side_length * sqrt(6.0f) / 3.0f, -side_length * sqrt(3.0f) / 2.0f / 3.0f);

    center = Vec3(side_length / 2.0f, side_length * sqrt(6.0f) / 3.0f * (1.0f / 4.0f), -side_length * sqrt(3.0f) / 2.0f / 3.0f);

    a -= center;
    b -= center;
    c -= center;
    d -= center;
    center -= center;

    triangles = {
        Triangle(c, b, a),
        Triangle(a, b, d),
        Triangle(b, c, d),
        Triangle(c, a, d),
    };
}

static std::vector<Triangle> subdivide(Triangle triangle)
{
    Vec3 new_vertex0 = (triangle.vertices[0].position + triangle.vertices[1].position) / 2.0f;
    Vec3 new_vertex1 = (triangle.vertices[1].position + triangle.vertices[2].position) / 2.0f;
    Vec3 new_vertex2 = (triangle.vertices[2].position + triangle.vertices[0].position) / 2.0f;

    Triangle new_triangle0 = Triangle(triangle.vertices[0].position, new_vertex0, new_vertex2);
    Triangle new_triangle1 = Triangle(new_vertex0, triangle.vertices[1].position, new_vertex1);
    Triangle new_triangle2 = Triangle(new_vertex0, new_vertex1, new_vertex2);
    Triangle new_triangle3 = Triangle(new_vertex2, new_vertex1, triangle.vertices[2].position);

    return {new_triangle0, new_triangle1, new_triangle2, new_triangle3};
}

static std::vector<Triangle> recursive_subdivide(const Triangle &triangle, int recursive_depth)
{
    if (recursive_depth == 0)
    {
        return {triangle};
    }

    Vec3 new_vertex0 = (triangle.vertices[0].position + triangle.vertices[1].position) / 2.0f;
    Vec3 new_vertex1 = (triangle.vertices[1].position + triangle.vertices[2].position) / 2.0f;
    Vec3 new_vertex2 = (triangle.vertices[2].position + triangle.vertices[0].position) / 2.0f;

    Triangle new_triangle0 = Triangle(triangle.vertices[0].position, new_vertex0, new_vertex2);
    Triangle new_triangle1 = Triangle(new_vertex0, triangle.vertices[1].position, new_vertex1);
    Triangle new_triangle2 = Triangle(new_vertex0, new_vertex1, new_vertex2);
    Triangle new_triangle3 = Triangle(new_vertex2, new_vertex1, triangle.vertices[2].position);

    std::vector<Triangle> ret;
    recursive_depth--;
    const std::vector<Triangle> &ret1 = recursive_subdivide(new_triangle0, recursive_depth);
    const std::vector<Triangle> &ret2 = recursive_subdivide(new_triangle1, recursive_depth);
    const std::vector<Triangle> &ret3 = recursive_subdivide(new_triangle2, recursive_depth);
    const std::vector<Triangle> &ret4 = recursive_subdivide(new_triangle3, recursive_depth);
    ret.insert(ret.end(), ret1.begin(), ret1.end());
    ret.insert(ret.end(), ret2.begin(), ret2.end());
    ret.insert(ret.end(), ret3.begin(), ret3.end());
    ret.insert(ret.end(), ret4.begin(), ret4.end());

    return ret;
}

std::shared_ptr<Mesh> create_icosphere_mesh(float radius, int regression_depth)
{
    std::vector<Triangle> m_triangles;
    Vec3 m_center;

    // 0. 创建正四面体
    create_tetrahedron(m_triangles, m_center);

    // 1. 对每个面细分
    for (int i = 0; i < regression_depth; i++)
    {
        std::vector<Triangle> new_triangles;
        for (int j = 0; j < m_triangles.size(); j++)
        {
            auto divided_triangles = subdivide(m_triangles[j]);
            new_triangles.insert(new_triangles.end(), divided_triangles.begin(), divided_triangles.end());
        }
        m_triangles = new_triangles;
    }

    // std::vector<Triangle> new_triangles;
    // for (int i = 0; i < m_triangles.size(); i++) {
    //     const auto& ret = recursive_subdivide(m_triangles[i], regression_depth);
    //     new_triangles.insert(new_triangles.end(), ret.begin(), ret.end());
    // }
    // m_triangles = new_triangles;

    // 2. 对所有细分顶点到中心的距离做归一化
    std::vector<Vertex> all_vertices;
    for (auto &triangle : m_triangles)
    {
        for (auto &vertex : triangle.vertices)
        {
            vertex.position = m_center + radius * Math::Normalize(vertex.position - m_center);
        }
        for (auto &vertex : triangle.vertices)
        {
            // Vec3 a = triangle.vertices[0] - triangle.vertices[1];
            // Vec3 b = triangle.vertices[0] - triangle.vertices[2];
            // Vec3 normal = Normalize(Cross(a, b));
            Vec3 normal = Math::Normalize(vertex.position - m_center);

            Vertex v;
            v.position = vertex.position;
            v.normal = normal;
            all_vertices.push_back(v);
        }
    }

    // TODO optimize index method
    std::vector<int> indices;
    indices.reserve(all_vertices.size() + 1);
    for (int i = 0; i < all_vertices.size(); i++)
    {
        indices.push_back(i);
    }

    // Logger::info("MeshAlgorithm::create_icosphere_mesh({}), time:{}", regression_depth, time);

    return std::make_shared<Mesh>(all_vertices, indices);
}

std::shared_ptr<Mesh> create_quad_mesh(const Point3 &origin, const Vec3 &positive_dir_u, const Vec3 &positive_dir_v)
{
    std::vector<Vertex> vertices;
    std::vector<int> indices;

    Vec3 normal = Math::Normalize(Math::Cross(positive_dir_u, positive_dir_v));
    Point3 origin_p = origin;
    Point3 right_p = origin_p + positive_dir_u;
    Point3 upper_right_p = origin_p + positive_dir_u + positive_dir_v;
    Point3 upper_p = origin_p + positive_dir_v;
    Point3 points[4] = {origin_p, right_p, upper_right_p, upper_p};

    for (int i = 0; i < 4; i++)
    {
        Vertex vertex;

        vertex.position = points[i];
        vertex.normal = normal;
        float u = (i == 0 || i == 3) ? 0.0f : 1.0f;
        float v = (i == 0 || i == 1) ? 0.0f : 1.0f;
        vertex.texture_uv = {u, v};

        vertices.push_back(vertex);
    }

    int cubeIndices[] =
        {
            0,
            1,
            2,
            0,
            2,
            3,
        };
    for (int i = 0; i < sizeof(cubeIndices) / sizeof(cubeIndices[0]); i++)
    {
        indices.push_back(cubeIndices[i]);
    }

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> create_complex_quad_mesh(const Vec2 &size)
{
    std::vector<Vertex> vertices;
    std::vector<int> indices;

    Point3 start_point = Point3(-size.x / 2.0f, 0.0f, size.y / 2.0f);
    Vec3 u = Vec3(-2.0f * start_point.x, 0, 0);
    Vec3 v = Vec3(0, 0, -2.0f * start_point.z);
    Point3 end_point = start_point + u + v;

    size_t sub_quad_num_u = (size_t)(size.x);
    size_t sub_quad_num_v = (size_t)(size.y);
    Vec3 sub_u = u / (float)sub_quad_num_u;
    Vec3 sub_v = v / (float)sub_quad_num_v;
    for (int i = 0; i < sub_quad_num_v; i++)
    {
        for (int j = 0; j < sub_quad_num_u; j++)
        {
            Point3 sub_start_point = start_point + (float)j * sub_u + (float)i * sub_v;
            Point3 sub_end_point = sub_start_point + sub_u + sub_v;
            std::shared_ptr<Mesh> sub_mesh_data = create_quad_mesh(sub_start_point, sub_u, sub_v);
            vertices.insert(vertices.end(), sub_mesh_data->vertices.begin(), sub_mesh_data->vertices.end());
            for (auto &index : sub_mesh_data->indices)
            {
                index += 4 * (j + i * sub_quad_num_u);
            }
            indices.insert(indices.end(), sub_mesh_data->indices.begin(), sub_mesh_data->indices.end());
            sub_mesh_data.reset();
        }
    }

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> create_screen_mesh()
{
    return create_quad_mesh(Point3(-1.0f, -1.0f, 0.0f), Vec3(2.0f, 0.0f, 0.0f), Vec3(0.0f, 2.0f, 0.0f));
}

}
