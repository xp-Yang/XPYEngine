#include "RenderSourceData.hpp"
#include "Render/RHI/rhi.hpp"

RenderTextureData::RenderTextureData(std::shared_ptr<Texture> texture_)
{
    if (texture_.get() == nullptr)
    {
        *this = RenderTextureData::defaultTexture();
        return;
    }

    const auto &rhi = Rhi::get();

    unsigned char *data = texture_->data;
    if (data)
    {
        RhiTexture::Format format;
        if (texture_->channel_count == 1)
            format = RhiTexture::Format::R8;
        else if (texture_->channel_count == 3)
            format = RhiTexture::Format::RGB8;
        else if (texture_->channel_count == 4)
            format = RhiTexture::Format::RGBA8;

        RhiTexture *texture = rhi->newTexture(format, Vec2(texture_->width, texture_->height), 1, RhiTexture::Flag::sRGB, data);
        texture->create();

        id = texture->id();
    }
    else
    {
        assert(false);
    }
}

RenderTextureData::RenderTextureData(std::shared_ptr<CubeTexture> cube_texture_)
{
    RhiTexture::Format format = RhiTexture::Format::RGB8;
    if (cube_texture_->channel_count == 4)
        format = RhiTexture::Format::RGBA8;

    RhiTexture* texture = Rhi::get()->newCubeTexture(format, Vec2(cube_texture_->width, cube_texture_->height), 1, RhiTexture::CubeMap, cube_texture_->datas);
    texture->create();
    id = texture->id();
}

RenderTextureData &RenderTextureData::defaultTexture()
{
    static std::shared_ptr<Texture> diffuse_texture = std::make_shared<Texture>(TextureType::Custom, std::string(RESOURCE_DIR) + "/images/default_map.png", false);
    static RenderTextureData texture(diffuse_texture);
    return texture;
}

RenderTextureData &RenderTextureData::defaultCubeTexture()
{
    static std::shared_ptr<CubeTexture> cube_texture;
    if (!cube_texture) {
        cube_texture = std::make_shared<CubeTexture>();
        cube_texture->width = 1;
        cube_texture->height = 1;
        cube_texture->channel_count = 3;
        static float neutral_depth = 1.0f;
        std::array<unsigned char*, 6> cube_data{};
        cube_texture->datas.fill(reinterpret_cast<unsigned char*>(&neutral_depth));
    }
    static RenderTextureData texture(cube_texture);
    return texture;
}

RenderMeshData::RenderMeshData(std::shared_ptr<Mesh> mesh_data)
{
    m_vertices_count = mesh_data->vertices.size();
    m_indices_count = mesh_data->indices.size();

    const auto &rhi = Rhi::get();

    RhiBuffer *vbuf = rhi->newBuffer(RhiBuffer::Immutable, RhiBuffer::VertexBuffer, (void *)(&(mesh_data->vertices[0])), mesh_data->vertices.size() * sizeof(Vertex));
    vbuf->create();
    // glGenBuffers(1, &m_VBO);
    // glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // glBufferData(GL_ARRAY_BUFFER, mesh_data->vertices().size() * sizeof(Vertex), &(mesh_data->vertices()[0]), GL_STATIC_DRAW);

    assert(!mesh_data->indices.empty());
    RhiBuffer *ibuf = rhi->newBuffer(RhiBuffer::Immutable, RhiBuffer::IndexBuffer, (void *)(&(mesh_data->indices[0])), mesh_data->indices.size() * sizeof(unsigned int));
    ibuf->create();
    // glGenBuffers(1, &m_IBO);
    ////glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    ////glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_data->indices().size() * sizeof(unsigned int), &(mesh_data->indices()[0]), GL_STATIC_DRAW);
    // glBindBuffer(GL_ARRAY_BUFFER, m_IBO);
    // glBufferData(GL_ARRAY_BUFFER, mesh_data->indices().size() * sizeof(unsigned int), &(mesh_data->indices()[0]), GL_STATIC_DRAW);

    m_vertex_layout = rhi->newVertexLayout(vbuf, ibuf);
    m_vertex_layout->setAttributes({
        {0, RhiVertexAttribute::Format::Float3, sizeof(Vertex), 0},                                  // position
        {1, RhiVertexAttribute::Format::Float3, sizeof(Vertex), offsetof(Vertex, normal)},           // normal
        {2, RhiVertexAttribute::Format::Float2, sizeof(Vertex), offsetof(Vertex, texture_uv)},       // uv
        {3, RhiVertexAttribute::Format::SInt4, sizeof(Vertex), offsetof(Vertex, bone_ids)},          // bone ids
        {4, RhiVertexAttribute::Format::Float4, sizeof(Vertex), offsetof(Vertex, bone_weights)},     // bone weights
    });
    m_vertex_layout->create();
    // glGenVertexArrays(1, &m_VAO);
    // glBindVertexArray(m_VAO);
    // glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texture_uv));
    // glEnableVertexAttribArray(2);
}

void RenderMeshData::reset()
{
    // if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
}

void RenderMeshData::create_instancing(void *instancing_data, int instancing_data_size, int buffer_capacity_size)
{
    if (instancing_data_size <= 0)
        return;

    const auto &rhi = Rhi::get();

    m_instancing_capacity_bytes = buffer_capacity_size > 0 ? buffer_capacity_size : instancing_data_size;
    m_instancing_buffer = rhi->newBuffer(RhiBuffer::Dynamic, RhiBuffer::VertexBuffer, nullptr, m_instancing_capacity_bytes);
    m_instancing_buffer->create();
    m_instancing_buffer->update(instancing_data, instancing_data_size);

    m_vertex_layout->setAttributes({
        {0, RhiVertexAttribute::Format::Float3, sizeof(Vertex), 0},                                  // position
        {1, RhiVertexAttribute::Format::Float3, sizeof(Vertex), offsetof(Vertex, normal)},           // normal
        {2, RhiVertexAttribute::Format::Float2, sizeof(Vertex), offsetof(Vertex, texture_uv)},       // uv
        {3, RhiVertexAttribute::Format::SInt4, sizeof(Vertex), offsetof(Vertex, bone_ids)},          // bone ids
        {4, RhiVertexAttribute::Format::Float4, sizeof(Vertex), offsetof(Vertex, bone_weights)},     // bone weights
        {5, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 0},                                // 矩阵
        {6, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), sizeof(Vec4)},                     
        {7, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 2 * sizeof(Vec4)},
        {8, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 3 * sizeof(Vec4)},
        {9, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 4 * sizeof(Vec4)},                 // color
    });
    m_vertex_layout->createInstancing(m_instancing_buffer, 5);
}

void RenderMeshData::update_instancing(void* instancing_data, int instancing_data_size)
{
    if (instancing_data_size <= 0 || instancing_data == nullptr)
        return;
    if (!m_instancing_buffer)
    {
        create_instancing(instancing_data, instancing_data_size);
        return;
    }
    if (instancing_data_size > m_instancing_capacity_bytes)
    {
        // TODO 旧的buffer需要释放
        int new_capacity = std::max(instancing_data_size, m_instancing_capacity_bytes * 2);
        create_instancing(instancing_data, instancing_data_size, new_capacity);
    }
    m_instancing_buffer->update(instancing_data, instancing_data_size);
}

RenderMaterialData::RenderMaterialData(std::shared_ptr<Material> material_)
{
    material_version = material_->version;
    alpha = material_->alpha;
    base_color_factor = material_->base_color_factor;
    metallic_factor = material_->metallic_factor;
    roughness_factor = material_->roughness_factor;
    ao_factor = material_->ao_factor;
    diffuse_factor = material_->diffuse_factor;
    specular_factor = material_->specular_factor;
    shininess = material_->shininess;

    // TODO 贴图更新了，texture数据的释放和加载
    if (albedo_map == 0)
    {
        albedo_map = RenderTextureData(material_->albedo_texture).id;
    }
    if (metallic_map == 0)
    {
        metallic_map = RenderTextureData(material_->metallic_texture).id;
    }
    if (roughness_map == 0)
    {
        roughness_map = RenderTextureData(material_->roughness_texture).id;
    }
    if (ao_map == 0)
    {
        ao_map = RenderTextureData(material_->ao_texture).id;
    }
    if (diffuse_map == 0)
    {
        diffuse_map = RenderTextureData(material_->diffuse_texture).id;
    }
    if (specular_map == 0)
    {
        specular_map = RenderTextureData(material_->specular_texture).id;
    }
    if (normal_map == 0)
    {
        normal_map = RenderTextureData(material_->normal_texture).id;
    }
    if (height_map == 0)
    {
        height_map = RenderTextureData(material_->height_texture).id;
    }
}

void RenderMeshNode::updateRenderMaterialData(std::shared_ptr<Material> material_)
{
    material.material_version = material_->version;
    material.alpha = material_->alpha;
    material.base_color_factor = material_->base_color_factor;
    material.metallic_factor = material_->metallic_factor;
    material.roughness_factor = material_->roughness_factor;
    material.ao_factor = material_->ao_factor;
    material.diffuse_factor = material_->diffuse_factor;
    material.specular_factor = material_->specular_factor;
    material.shininess = material_->shininess;

    // TODO 贴图更新了，texture数据的释放和加载
    if (material.albedo_map == 0)
    {
        GL_HANDLE albedo_texture = RenderTextureData(material_->albedo_texture).id;
        material.albedo_map = albedo_texture;
    }
    if (material.metallic_map == 0)
    {
        GL_HANDLE metallic_map = RenderTextureData(material_->metallic_texture).id;
        material.metallic_map = metallic_map;
    }
    if (material.roughness_map == 0)
    {
        GL_HANDLE roughness_map = RenderTextureData(material_->roughness_texture).id;
        material.roughness_map = roughness_map;
    }
    if (material.ao_map == 0)
    {
        GL_HANDLE ao_map = RenderTextureData(material_->ao_texture).id;
        material.ao_map = ao_map;
    }

    if (material.diffuse_map == 0)
    {
        GL_HANDLE diffuse_map = RenderTextureData(material_->diffuse_texture).id;
        material.diffuse_map = diffuse_map;
    }
    if (material.specular_map == 0)
    {
        GL_HANDLE specular_map = RenderTextureData(material_->specular_texture).id;
        material.specular_map = specular_map;
    }
    if (material.normal_map == 0)
    {
        GL_HANDLE normal_map = RenderTextureData(material_->normal_texture).id;
        material.normal_map = normal_map;
    }
    if (material.height_map == 0)
    {
        GL_HANDLE height_map = RenderTextureData(material_->height_texture).id;
        material.height_map = height_map;
    }
}
