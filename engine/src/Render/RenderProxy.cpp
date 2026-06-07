#include "Render/RenderProxy.hpp"

#include <utility>

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
            mesh_section->world_bounds = transformRenderAABB(mesh_section->local_bounds, mesh_section->model_matrix);
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
    section->world_bounds = transformRenderAABB(section->local_bounds, section->model_matrix);
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
