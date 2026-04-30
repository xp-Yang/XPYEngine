#include "RenderSourceData.hpp"
#include "Render/RHI/rhi.hpp"

RenderTextureData::RenderTextureData(std::shared_ptr<Texture> texture_)
{
    if (texture_.get() == nullptr)
    {
        *this = RenderTextureData::defaultTexture();
        return;
    }

    const auto &rhi = RenderSourceData::rhi;

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

RenderTextureData::RenderTextureData(const CubeTexture &cube_texture_)
{
    id = RenderSourceData::rhi->newCubeTexture(cube_texture_.width, cube_texture_.height, cube_texture_.datas);
}

RenderTextureData &RenderTextureData::defaultTexture()
{
    static std::shared_ptr<Texture> diffuse_texture = std::make_shared<Texture>(TextureType::Custom, std::string(RESOURCE_DIR) + "/images/default_map.png", false);
    static RenderTextureData texture(diffuse_texture);
    return texture;
}

RenderMeshData::RenderMeshData(std::shared_ptr<Mesh> mesh_data)
{
    m_vertices_count = mesh_data->vertices.size();
    m_indices_count = mesh_data->indices.size();

    const auto &rhi = RenderSourceData::rhi;

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

void RenderMeshData::create_instancing(void *instancing_data, int instancing_data_size)
{
    const auto &rhi = RenderSourceData::rhi;

    RhiBuffer *inst_buf = rhi->newBuffer(RhiBuffer::Immutable, RhiBuffer::VertexBuffer, instancing_data, instancing_data_size);
    inst_buf->create();

    m_vertex_layout->setAttributes({
        {0, RhiVertexAttribute::Format::Float3, sizeof(Vertex), 0},                            // position
        {1, RhiVertexAttribute::Format::Float3, sizeof(Vertex), offsetof(Vertex, normal)},     // normal
        {2, RhiVertexAttribute::Format::Float2, sizeof(Vertex), offsetof(Vertex, texture_uv)}, // uv
        {3, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 0},
        {4, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), sizeof(Vec4)},
        {5, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 2 * sizeof(Vec4)},
        {6, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 3 * sizeof(Vec4)},
        {7, RhiVertexAttribute::Format::Float4, 5 * sizeof(Vec4), 4 * sizeof(Vec4)},
    });
    m_vertex_layout->createInstancing(inst_buf, 3);
}

RenderMaterialData::RenderMaterialData(std::shared_ptr<Material> material_)
{
    alpha = material_->alpha;
    //// temp
    // albedo = material_->albedo;
    // metallic = material_->metallic;
    // roughness = material_->roughness;
    // ao = material_->ao;

    // TODO 贴图更新了，texture数据的释放和加载
    if (diffuse_map == 0)
    {
        unsigned int diffuse_map = RenderTextureData(material_->diffuse_texture).id;
        diffuse_map = diffuse_map;
    }
    if (specular_map == 0)
    {
        unsigned int specular_map = RenderTextureData(material_->specular_texture).id;
        specular_map = specular_map;
    }
    if (normal_map == 0)
    {
        unsigned int normal_map = RenderTextureData(material_->normal_texture).id;
        normal_map = normal_map;
    }
    if (height_map == 0)
    {
        unsigned int height_map = RenderTextureData(material_->height_texture).id;
        height_map = height_map;
    }
}

void RenderMeshNode::updateRenderMaterialData(std::shared_ptr<Material> material_)
{
    material.alpha = material_->alpha;
    //// temp
    // material.albedo = material_->albedo;
    // material.metallic = material_->metallic;
    // material.roughness = material_->roughness;
    // material.ao = material_->ao;

    // TODO 贴图更新了，texture数据的释放和加载
    if (material.albedo_map == 0)
    {
        unsigned int albedo_texture = RenderTextureData(material_->albedo_texture).id;
        material.albedo_map = albedo_texture;
    }
    if (material.metallic_map == 0)
    {
        unsigned int metallic_map = RenderTextureData(material_->metallic_texture).id;
        material.metallic_map = metallic_map;
    }
    if (material.roughness_map == 0)
    {
        unsigned int roughness_map = RenderTextureData(material_->roughness_texture).id;
        material.roughness_map = roughness_map;
    }
    if (material.ao_map == 0)
    {
        unsigned int ao_map = RenderTextureData(material_->ao_texture).id;
        material.ao_map = ao_map;
    }

    if (material.diffuse_map == 0)
    {
        unsigned int diffuse_map = RenderTextureData(material_->diffuse_texture).id;
        material.diffuse_map = diffuse_map;
    }
    if (material.specular_map == 0)
    {
        unsigned int specular_map = RenderTextureData(material_->specular_texture).id;
        material.specular_map = specular_map;
    }
    if (material.normal_map == 0)
    {
        unsigned int normal_map = RenderTextureData(material_->normal_texture).id;
        material.normal_map = normal_map;
    }
    if (material.height_map == 0)
    {
        unsigned int height_map = RenderTextureData(material_->height_texture).id;
        material.height_map = height_map;
    }
}
