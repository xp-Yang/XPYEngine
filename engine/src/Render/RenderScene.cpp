#include "Render/RenderScene.hpp"
#include "Render/RenderFrameData.hpp"
#include "Render/RHI/rhi.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <unordered_map>

namespace
{
std::unordered_map<const MeshGeometry*, std::weak_ptr<RenderGeometryGpuResource>> s_render_geometry_gpu_cache;

RenderAABB transformAABB(const RenderAABB& bounds, const Mat4& matrix)
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

        texture = rhi->newTexture(format, Vec2(texture_->width, texture_->height), 1, RhiTexture::Flag::sRGB, data);
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

    texture = Rhi::get()->newCubeTexture(format, Vec2(cube_texture_->width, cube_texture_->height), 1, RhiTexture::CubeMap, cube_texture_->datas);
    texture->create();
    id = texture->id();
}

RenderTextureData &RenderTextureData::defaultTexture()
{
    static std::shared_ptr<Texture> diffuse_texture = std::make_shared<Texture>(TextureType::Custom, std::string(ASSET_DIR) + "/images/default_map.png", false);
    static RenderTextureData texture(diffuse_texture);
    return texture;
}

RenderTextureData &RenderTextureData::defaultCubeTexture()
{
    static std::shared_ptr<CubeTexture> cube_texture;
    if (!cube_texture)
    {
        cube_texture = std::make_shared<CubeTexture>();
        cube_texture->width = 1;
        cube_texture->height = 1;
        cube_texture->channel_count = 3;
        static float neutral_depth = 1.0f;
        std::array<unsigned char *, 6> cube_data{};
        cube_texture->datas.fill(reinterpret_cast<unsigned char *>(&neutral_depth));
    }
    static RenderTextureData texture(cube_texture);
    return texture;
}

RenderMeshResource::RenderMeshResource(std::shared_ptr<Mesh> mesh_data)
{
    assert(mesh_data && mesh_data->geometry);

    m_geometry_resource = renderGeometryResourceOf(mesh_data->geometry);
    m_vertices_count = m_geometry_resource->vertices_count;
    m_indices_count = m_geometry_resource->indices_count;

    const auto &rhi = Rhi::get();

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

void RenderMeshResource::create_instancing(void *instancing_data, int instancing_data_size, int buffer_capacity_size)
{
    if (instancing_data_size <= 0)
        return;

    const auto &rhi = Rhi::get();

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

void RenderMeshResource::update_instancing(void *instancing_data, int instancing_data_size)
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

    // TODO reload texture handles when the source material changes texture assets.
    if (!albedo_map)
        albedo_map = RenderTextureData(material_->albedo_texture).texture;
    if (!metallic_map)
        metallic_map = RenderTextureData(material_->metallic_texture).texture;
    if (!roughness_map)
        roughness_map = RenderTextureData(material_->roughness_texture).texture;
    if (!ao_map)
        ao_map = RenderTextureData(material_->ao_texture).texture;

    if (!diffuse_map)
        diffuse_map = RenderTextureData(material_->diffuse_texture).texture;
    if (!specular_map)
        specular_map = RenderTextureData(material_->specular_texture).texture;
    if (!normal_map)
        normal_map = RenderTextureData(material_->normal_texture).texture;
    // 仅当材质拥有“真实”法线贴图时才扰动法线。create_complete_default_material /
    // fillBlinnPhongFromPBR 会把 normal_texture 填成默认白图 pure_white_map.png（非空且非平面法线），
    // 直接按非空判断会把白图当法线图采样 (1,1,1)，导致整面法线歪斜、镜像 UV 中缝裂开。
    static const std::string kDefaultNormalPath = std::string(ASSET_DIR) + "/images/pure_white_map.png";
    has_normal_map = material_->normal_texture
        && material_->normal_texture->texture_filepath != kDefaultNormalPath;
    if (!height_map)
        height_map = RenderTextureData(material_->height_texture).texture;
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
    , world_bounds(transformAABB(local_bounds_, matrix))
    , source_index_offset(source_index_offset_)
    , source_index_count(source_index_count_)
{
}

void RenderMeshSection::updateRenderMaterial(std::shared_ptr<Material> material_)
{
    material.updateFrom(material_);
}

RenderObjectProxy::RenderObjectProxy(GObjectID object_id)
    : m_object_id(object_id)
{
}

void RenderObjectProxy::setModelMatrix(const Mat4& model_matrix)
{
    m_model_matrix = model_matrix;
    for (auto& mesh_section : m_mesh_sections)
    {
        if (mesh_section)
        {
            mesh_section->model_matrix = m_model_matrix * mesh_section->local_matrix;
            mesh_section->world_bounds = transformAABB(mesh_section->local_bounds, mesh_section->model_matrix);
        }
    }
}

void RenderObjectProxy::setVisible(bool visible)
{
    m_visible = visible;
    for (auto& mesh_section : m_mesh_sections)
    {
        if (mesh_section)
            mesh_section->visible = visible;
    }
}

RenderMeshSection* RenderObjectProxy::addMeshSection(std::unique_ptr<RenderMeshSection> section)
{
    if (!section)
        return nullptr;

    section->owner = this;
    section->visible = m_visible;
    section->world_bounds = transformAABB(section->local_bounds, section->model_matrix);
    RenderMeshSection* raw_section = section.get();
    m_mesh_sections.push_back(std::move(section));
    return raw_section;
}

RenderMeshSection* RenderObjectProxy::meshSection(int sub_mesh_idx)
{
    for (auto& mesh_section : m_mesh_sections)
    {
        if (mesh_section && mesh_section->section_id.sub_mesh_idx == sub_mesh_idx)
            return mesh_section.get();
    }
    return nullptr;
}

const RenderMeshSection* RenderObjectProxy::meshSection(int sub_mesh_idx) const
{
    for (const auto& mesh_section : m_mesh_sections)
    {
        if (mesh_section && mesh_section->section_id.sub_mesh_idx == sub_mesh_idx)
            return mesh_section.get();
    }
    return nullptr;
}

bool RenderObjectProxy::hasVisibleStaticShadowCaster() const
{
    if (!m_visible)
        return false;

    for (const auto& mesh_section : m_mesh_sections)
    {
        if (mesh_section && mesh_section->visible && mesh_section->static_shadow_caster && !mesh_section->use_skinning)
            return true;
    }
    return false;
}

RenderObjectProxy* RenderScene::objectProxy(GObjectID object_id)
{
    auto it = m_object_proxies.find(object_id);
    return it == m_object_proxies.end() ? nullptr : it->second.get();
}

const RenderObjectProxy* RenderScene::objectProxy(GObjectID object_id) const
{
    auto it = m_object_proxies.find(object_id);
    return it == m_object_proxies.end() ? nullptr : it->second.get();
}

RenderMeshSection* RenderScene::meshSection(const RenderMeshSectionID& id)
{
    RenderObjectProxy* proxy = objectProxy(id.object_id);
    return proxy ? proxy->meshSection(id.sub_mesh_idx) : nullptr;
}

const RenderMeshSection* RenderScene::meshSection(const RenderMeshSectionID& id) const
{
    const RenderObjectProxy* proxy = objectProxy(id.object_id);
    return proxy ? proxy->meshSection(id.sub_mesh_idx) : nullptr;
}

RenderMeshSection* RenderScene::addMeshSection(const RenderMeshSectionID& id, std::unique_ptr<RenderMeshSection> section)
{
    if (!section)
        return nullptr;

    auto it = m_object_proxies.find(id.object_id);
    if (it == m_object_proxies.end())
        it = m_object_proxies.emplace(id.object_id, std::make_unique<RenderObjectProxy>(id.object_id)).first;

    return it->second->addMeshSection(std::move(section));
}

void RenderScene::removeObjectProxy(GObjectID object_id)
{
    if (const RenderObjectProxy* proxy = objectProxy(object_id))
    {
        if (proxy->hasVisibleStaticShadowCaster())
            ++m_shadow_static_version;
    }
    m_object_proxies.erase(object_id);
}

void RenderScene::removeDeadObjectProxies(const std::unordered_set<GObjectID>& alive_object_ids)
{
    for (auto it = m_object_proxies.begin(); it != m_object_proxies.end();)
    {
        if (alive_object_ids.find(it->first) == alive_object_ids.end())
        {
            if (it->second && it->second->hasVisibleStaticShadowCaster())
                ++m_shadow_static_version;
            it = m_object_proxies.erase(it);
        }
        else
            ++it;
    }
}

void RenderScene::removeDeadSubMeshesOfObject(GObjectID object_id, const std::unordered_set<int>& alive_sub_mesh_ids)
{
    RenderObjectProxy* proxy = objectProxy(object_id);
    if (!proxy)
        return;

    const bool affected_static_shadow = proxy->hasVisibleStaticShadowCaster();
    auto& mesh_sections = proxy->meshSections();
    mesh_sections.erase(
        std::remove_if(
            mesh_sections.begin(),
            mesh_sections.end(),
            [&alive_sub_mesh_ids](const std::unique_ptr<RenderMeshSection>& section)
            {
                return !section || alive_sub_mesh_ids.find(section->section_id.sub_mesh_idx) == alive_sub_mesh_ids.end();
            }),
        mesh_sections.end());

    if (mesh_sections.empty())
        removeObjectProxy(object_id);
    else if (affected_static_shadow)
        ++m_shadow_static_version;
}

void RenderScene::setObjectVisible(GObjectID object_id, bool visible)
{
    RenderObjectProxy* proxy = objectProxy(object_id);
    if (proxy)
    {
        const bool affected_static_shadow = proxy->hasVisibleStaticShadowCaster();
        proxy->setVisible(visible);
        if (affected_static_shadow || proxy->hasVisibleStaticShadowCaster())
            ++m_shadow_static_version;
    }
}

void RenderScene::updateObjectTransform(GObjectID object_id, const Mat4& model_matrix)
{
    RenderObjectProxy* proxy = objectProxy(object_id);
    if (proxy)
    {
        const bool affected_static_shadow = proxy->hasVisibleStaticShadowCaster();
        proxy->setModelMatrix(model_matrix);
        if (affected_static_shadow)
            ++m_shadow_static_version;
    }
}

void RenderScene::updateObjectMaterials(GObject& object)
{
    RenderObjectProxy* proxy = objectProxy(object.ID());
    MeshComponent* mesh_component = object.getComponent<MeshComponent>();
    if (!proxy || !mesh_component)
        return;

    // TODO: This round tracks mesh/material dirty at object granularity. Later, carry
    // sub_mesh_idx in RenderSceneChange so a material edit can refresh only the
    // affected RenderMeshSection.
    for (const auto& sub_mesh : mesh_component->sub_meshes)
    {
        if (!sub_mesh || !sub_mesh->material)
            continue;

        if (RenderMeshSection* section = proxy->meshSection(sub_mesh->sub_mesh_idx))
            section->updateRenderMaterial(sub_mesh->material);
    }
}

void RenderScene::rebuildMeshSectionLists()
{
    m_visible_sections.clear();
    m_opaque_sections.clear();
    m_transparent_sections.clear();
    m_skinned_sections.clear();
    m_static_shadow_caster_sections.clear();
    m_dynamic_shadow_caster_sections.clear();
    m_has_transparent = false;

    size_t mesh_section_count = 0;
    for (const auto& pair : m_object_proxies)
        mesh_section_count += pair.second ? pair.second->meshSections().size() : 0;

    m_visible_sections.reserve(mesh_section_count);
    m_opaque_sections.reserve(mesh_section_count);
    m_transparent_sections.reserve(mesh_section_count);
    m_skinned_sections.reserve(mesh_section_count);
    m_static_shadow_caster_sections.reserve(mesh_section_count);
    m_dynamic_shadow_caster_sections.reserve(mesh_section_count);

    for (auto& pair : m_object_proxies)
    {
        RenderObjectProxy* proxy = pair.second.get();
        if (!proxy || !proxy->visible())
            continue;

        for (auto& mesh_section : proxy->meshSections())
        {
            RenderMeshSection* section = mesh_section.get();
            if (!section || !section->visible)
                continue;

            m_visible_sections.push_back(section);
            if (section->material.isTransparent())
            {
                m_transparent_sections.push_back(section);
                m_has_transparent = true;
            }
            else
            {
                m_opaque_sections.push_back(section);
            }

            if (section->use_skinning)
                m_skinned_sections.push_back(section);

            if (section->static_shadow_caster && !section->use_skinning)
                m_static_shadow_caster_sections.push_back(section);
            else
                m_dynamic_shadow_caster_sections.push_back(section);
        }
    }
    ++m_shadow_static_version;
}

void RenderScene::clearObjectProxies()
{
    m_object_proxies.clear();
    m_visible_sections.clear();
    m_opaque_sections.clear();
    m_transparent_sections.clear();
    m_skinned_sections.clear();
    m_static_shadow_caster_sections.clear();
    m_dynamic_shadow_caster_sections.clear();
    m_has_transparent = false;
    ++m_shadow_static_version;
}

void RenderScene::clear()
{
    clearObjectProxies();
    m_skybox.mesh.reset();
    m_skybox.skybox_cube_map = nullptr;
}

void RenderFrameData::reset()
{
    directional_lights.clear();
    point_lights.clear();
    picked_ids.clear();
    point_light_inst_amount = 0;
}
