#include "Render/RenderPrimitive.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

namespace
{
std::unordered_map<const MeshGeometry*, std::weak_ptr<RenderGeometryGpuResource>> s_render_geometry_gpu_cache;
std::unordered_map<const Texture*, std::weak_ptr<RenderTextureResource>> s_render_texture_cache;
std::unordered_map<const CubeTexture*, std::weak_ptr<RenderTextureResource>> s_render_cube_texture_cache;

std::shared_ptr<RenderGeometryGpuResource> renderGeometryResourceOf(const std::shared_ptr<MeshGeometry>& geometry)
{
    assert(geometry && !geometry->vertices.empty() && !geometry->indices.empty());

    auto cache_it = s_render_geometry_gpu_cache.find(geometry.get());
    if (cache_it != s_render_geometry_gpu_cache.end())
    {
        if (auto cached = cache_it->second.lock())
            return cached;
        s_render_geometry_gpu_cache.erase(cache_it);
    }

    const auto& rhi = Rhi::get();
    auto resource = std::make_shared<RenderGeometryGpuResource>();
    resource->vertices_count = geometry->vertices.size();
    resource->indices_count = geometry->indices.size();

    resource->vertex_buffer = rhi->newBuffer(
        RhiBuffer::Immutable,
        RhiBuffer::VertexBuffer,
        (void*)geometry->vertices.data(),
        static_cast<int>(geometry->vertices.size() * sizeof(Vertex)));
    resource->vertex_buffer->create();

    resource->index_buffer = rhi->newBuffer(
        RhiBuffer::Immutable,
        RhiBuffer::IndexBuffer,
        (void*)geometry->indices.data(),
        static_cast<int>(geometry->indices.size() * sizeof(unsigned int)));
    resource->index_buffer->create();

    s_render_geometry_gpu_cache[geometry.get()] = resource;
    return resource;
}
}

RenderAABB transformRenderAABB(const RenderAABB& bounds, const Mat4& matrix)
{
    if (!bounds.valid)
        return {};

    const std::array<Vec3, 8> corners = {
        Vec3(bounds.min.x, bounds.min.y, bounds.min.z),
        Vec3(bounds.max.x, bounds.min.y, bounds.min.z),
        Vec3(bounds.min.x, bounds.max.y, bounds.min.z),
        Vec3(bounds.max.x, bounds.max.y, bounds.min.z),
        Vec3(bounds.min.x, bounds.min.y, bounds.max.z),
        Vec3(bounds.max.x, bounds.min.y, bounds.max.z),
        Vec3(bounds.min.x, bounds.max.y, bounds.max.z),
        Vec3(bounds.max.x, bounds.max.y, bounds.max.z),
    };

    RenderAABB result;
    result.min = Vec3(std::numeric_limits<float>::max());
    result.max = Vec3(std::numeric_limits<float>::lowest());
    result.valid = true;
    for (const Vec3& corner : corners)
    {
        const Vec4 transformed = matrix * Vec4(corner, 1.0f);
        const Vec3 point = Vec3(transformed) / transformed.w;
        result.min = glm::min(result.min, point);
        result.max = glm::max(result.max, point);
    }
    return result;
}

RenderGeometryGpuResource::~RenderGeometryGpuResource()
{
    if (vertex_buffer)
    {
        vertex_buffer->destroy();
        delete vertex_buffer;
        vertex_buffer = nullptr;
    }
    if (index_buffer)
    {
        index_buffer->destroy();
        delete index_buffer;
        index_buffer = nullptr;
    }
}

RenderTextureResource::RenderTextureResource(std::shared_ptr<Texture> texture_)
    : m_source_texture(std::move(texture_))
{
    assert(m_source_texture);

    const auto& rhi = Rhi::get();

    unsigned char* data = m_source_texture->data;
    if (data)
    {
        RhiTexture::Format format = RhiTexture::Format::RGB8;
        if (m_source_texture->channel_count == 1)
            format = RhiTexture::Format::R8;
        else if (m_source_texture->channel_count == 3)
            format = RhiTexture::Format::RGB8;
        else if (m_source_texture->channel_count == 4)
            format = RhiTexture::Format::RGBA8;

        m_texture = rhi->newTexture(format, Vec2(m_source_texture->width, m_source_texture->height), 1, RhiTexture::Flag::sRGB, data);
        m_texture->create();
    }
    else
    {
        assert(false);
    }
}

RenderTextureResource::RenderTextureResource(std::shared_ptr<CubeTexture> cube_texture_)
    : m_source_cube_texture(std::move(cube_texture_))
{
    assert(m_source_cube_texture);

    RhiTexture::Format format = RhiTexture::Format::RGB8;
    if (m_source_cube_texture->channel_count == 4)
        format = RhiTexture::Format::RGBA8;

    m_texture = Rhi::get()->newCubeTexture(format, Vec2(m_source_cube_texture->width, m_source_cube_texture->height), 1, RhiTexture::CubeMap, m_source_cube_texture->datas);
    m_texture->create();
}

RenderTextureResource::~RenderTextureResource()
{
    if (m_texture)
    {
        m_texture->destroy();
        delete m_texture;
        m_texture = nullptr;
    }
}

std::shared_ptr<RenderTextureResource> RenderTextureResource::textureOf(std::shared_ptr<Texture> texture_)
{
    if (!texture_)
        return defaultTexture();

    auto cache_it = s_render_texture_cache.find(texture_.get());
    if (cache_it != s_render_texture_cache.end())
    {
        if (auto cached = cache_it->second.lock())
            return cached;
        s_render_texture_cache.erase(cache_it);
    }

    auto texture_data = std::make_shared<RenderTextureResource>(texture_);
    s_render_texture_cache[texture_.get()] = texture_data;
    return texture_data;
}

std::shared_ptr<RenderTextureResource> RenderTextureResource::cubeTextureOf(std::shared_ptr<CubeTexture> cube_texture_)
{
    if (!cube_texture_)
        return defaultCubeTexture();

    auto cache_it = s_render_cube_texture_cache.find(cube_texture_.get());
    if (cache_it != s_render_cube_texture_cache.end())
    {
        if (auto cached = cache_it->second.lock())
            return cached;
        s_render_cube_texture_cache.erase(cache_it);
    }

    auto texture_data = std::make_shared<RenderTextureResource>(cube_texture_);
    s_render_cube_texture_cache[cube_texture_.get()] = texture_data;
    return texture_data;
}

std::shared_ptr<RenderTextureResource> RenderTextureResource::defaultTexture()
{
    static std::shared_ptr<Texture> diffuse_texture = std::make_shared<Texture>(TextureType::Custom, std::string(ASSET_DIR) + "/images/default_map.png", false);
    static std::shared_ptr<RenderTextureResource> texture = std::make_shared<RenderTextureResource>(diffuse_texture);
    return texture;
}

std::shared_ptr<RenderTextureResource> RenderTextureResource::defaultCubeTexture()
{
    static std::shared_ptr<CubeTexture> cube_texture;
    if (!cube_texture)
    {
        cube_texture = std::make_shared<CubeTexture>();
        cube_texture->width = 1;
        cube_texture->height = 1;
        cube_texture->channel_count = 3;
        static float neutral_depth = 1.0f;
        cube_texture->datas.fill(reinterpret_cast<unsigned char*>(&neutral_depth));
    }
    static std::shared_ptr<RenderTextureResource> texture = std::make_shared<RenderTextureResource>(cube_texture);
    return texture;
}

RenderMeshResource::RenderMeshResource(std::shared_ptr<Mesh> mesh_data)
{
    assert(mesh_data && mesh_data->geometry);

    m_geometry_resource = renderGeometryResourceOf(mesh_data->geometry);
    m_vertices_count = m_geometry_resource->vertices_count;
    m_indices_count = m_geometry_resource->indices_count;

    const auto& rhi = Rhi::get();

    m_vertex_layout = rhi->newVertexLayout(m_geometry_resource->vertex_buffer, m_geometry_resource->index_buffer);
    m_vertex_layout->setAttributes({
        {0, RhiVertexAttribute::Format::Float3, sizeof(Vertex), 0},                              // position
        {1, RhiVertexAttribute::Format::Float3, sizeof(Vertex), offsetof(Vertex, normal)},       // normal
        {2, RhiVertexAttribute::Format::Float2, sizeof(Vertex), offsetof(Vertex, texture_uv)},   // uv
        {3, RhiVertexAttribute::Format::SInt4, sizeof(Vertex), offsetof(Vertex, bone_ids)},      // bone ids
        {4, RhiVertexAttribute::Format::Float4, sizeof(Vertex), offsetof(Vertex, bone_weights)}, // bone weights
        {5, RhiVertexAttribute::Format::Float4, sizeof(Vertex), offsetof(Vertex, tangent)},      // tangent (xyz) + handedness (w)
    });
    m_vertex_layout->create();
}

RenderMeshResource::RenderMeshResource(RenderMeshResource&& other) noexcept
    : m_geometry_resource(std::move(other.m_geometry_resource))
    , m_vertex_layout(other.m_vertex_layout)
    , m_vertices_count(other.m_vertices_count)
    , m_indices_count(other.m_indices_count)
    , m_instancing_buffer(other.m_instancing_buffer)
    , m_instancing_capacity_bytes(other.m_instancing_capacity_bytes)
{
    other.m_vertex_layout = nullptr;
    other.m_vertices_count = 0;
    other.m_indices_count = 0;
    other.m_instancing_buffer = nullptr;
    other.m_instancing_capacity_bytes = 0;
}

RenderMeshResource& RenderMeshResource::operator=(RenderMeshResource&& other) noexcept
{
    if (this != &other)
    {
        reset();
        m_geometry_resource = std::move(other.m_geometry_resource);
        m_vertex_layout = other.m_vertex_layout;
        m_vertices_count = other.m_vertices_count;
        m_indices_count = other.m_indices_count;
        m_instancing_buffer = other.m_instancing_buffer;
        m_instancing_capacity_bytes = other.m_instancing_capacity_bytes;

        other.m_vertex_layout = nullptr;
        other.m_vertices_count = 0;
        other.m_indices_count = 0;
        other.m_instancing_buffer = nullptr;
        other.m_instancing_capacity_bytes = 0;
    }
    return *this;
}

void RenderMeshResource::reset()
{
    if (m_vertex_layout)
    {
        m_vertex_layout->destroy();
        delete m_vertex_layout;
        m_vertex_layout = nullptr;
    }
    if (m_instancing_buffer)
    {
        m_instancing_buffer->destroy();
        delete m_instancing_buffer;
        m_instancing_buffer = nullptr;
    }
    m_instancing_capacity_bytes = 0;
    m_geometry_resource.reset();
    m_vertices_count = 0;
    m_indices_count = 0;
}

void RenderMeshResource::create_instancing(void* instancing_data, int instancing_data_size, int buffer_capacity_size)
{
    if (instancing_data_size <= 0)
        return;

    const auto& rhi = Rhi::get();

    if (m_instancing_buffer)
    {
        m_instancing_buffer->destroy();
        delete m_instancing_buffer;
        m_instancing_buffer = nullptr;
    }

    m_instancing_capacity_bytes = buffer_capacity_size > 0 ? buffer_capacity_size : instancing_data_size;
    m_instancing_buffer = rhi->newBuffer(RhiBuffer::Dynamic, RhiBuffer::VertexBuffer, nullptr, m_instancing_capacity_bytes);
    m_instancing_buffer->create();
    m_instancing_buffer->update(instancing_data, instancing_data_size);

    m_vertex_layout->createInstancing(m_instancing_buffer, {
        {5, RhiVertexAttribute::Format::Float4, 4 * sizeof(Vec4) + sizeof(Vec3), 0},                // matrix
        {6, RhiVertexAttribute::Format::Float4, 4 * sizeof(Vec4) + sizeof(Vec3), sizeof(Vec4)},     // matrix
        {7, RhiVertexAttribute::Format::Float4, 4 * sizeof(Vec4) + sizeof(Vec3), 2 * sizeof(Vec4)}, // matrix
        {8, RhiVertexAttribute::Format::Float4, 4 * sizeof(Vec4) + sizeof(Vec3), 3 * sizeof(Vec4)}, // matrix
        {9, RhiVertexAttribute::Format::Float3, 4 * sizeof(Vec4) + sizeof(Vec3), 4 * sizeof(Vec4)}, // color
    });
}

void RenderMeshResource::update_instancing(void* instancing_data, int instancing_data_size)
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
        const int new_capacity = std::max(instancing_data_size, m_instancing_capacity_bytes * 2);
        create_instancing(instancing_data, instancing_data_size, new_capacity);
    }
    m_instancing_buffer->update(instancing_data, instancing_data_size);
}

RenderMaterialResource::RenderMaterialResource(std::shared_ptr<Material> material_)
{
    updateFrom(material_);
}

void RenderMaterialResource::updateFrom(std::shared_ptr<Material> material_)
{
    if (!material_)
        return;

    material_version = material_->version;

    alpha = material_->alpha;

    base_color_factor = material_->base_color_factor;
    metallic_factor = material_->metallic_factor;
    roughness_factor = material_->roughness_factor;
    ao_factor = material_->ao_factor;

    diffuse_factor = material_->diffuse_factor;
    specular_factor = material_->specular_factor;
    shininess = material_->shininess;

    m_albedo_map = RenderTextureResource::textureOf(material_->albedo_texture);
    m_metallic_map = RenderTextureResource::textureOf(material_->metallic_texture);
    m_roughness_map = RenderTextureResource::textureOf(material_->roughness_texture);
    m_ao_map = RenderTextureResource::textureOf(material_->ao_texture);

    m_diffuse_map = RenderTextureResource::textureOf(material_->diffuse_texture);
    m_specular_map = RenderTextureResource::textureOf(material_->specular_texture);
    m_normal_map = RenderTextureResource::textureOf(material_->normal_texture);
    // 仅当材质拥有“真实”法线贴图时才扰动法线。create_complete_default_material /
    // fillBlinnPhongFromPBR 会把 normal_texture 填成默认白图 pure_white_map.png（非空且非平面法线），
    // 直接按非空判断会把白图当法线图采样 (1,1,1)，导致整面法线歪斜、镜像 UV 中缝裂开。
    static const std::string kDefaultNormalPath = std::string(ASSET_DIR) + "/images/pure_white_map.png";
    has_normal_map = material_->normal_texture
        && material_->normal_texture->texture_filepath != kDefaultNormalPath;
    m_height_map = RenderTextureResource::textureOf(material_->height_texture);
}

RenderMeshSection::RenderMeshSection(
    const RenderMeshSectionID& id,
    RenderMeshResource&& mesh_data,
    const RenderMaterialResource& material_data,
    Mat4 matrix,
    const RenderAABB& local_bounds_,
    int source_index_offset_,
    int source_index_count_)
    : section_id(id)
    , mesh(std::move(mesh_data))
    , material(material_data)
    , model_matrix(matrix)
    , local_bounds(local_bounds_)
    , world_bounds(transformRenderAABB(local_bounds_, matrix))
    , source_index_offset(source_index_offset_)
    , source_index_count(source_index_count_)
{
}

void RenderMeshSection::updateRenderMaterial(std::shared_ptr<Material> material_)
{
    material.updateFrom(material_);
}
