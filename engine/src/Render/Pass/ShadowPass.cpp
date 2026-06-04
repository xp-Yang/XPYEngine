#include "ShadowPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"
#include "Render/RenderCulling.hpp"

#include <algorithm>
#include <array>

namespace
{
    bool shouldDrawPointShadowCaster(
        const RenderMeshSection* section,
        const Vec3& light_position,
        float light_radius,
        const RenderFrustum& face_frustum)
    {
        if (!section)
            return false;
        if (!RenderCulling::aabbIntersectsSphere(section->world_bounds, light_position, light_radius))
            return false;
        if (section->use_skinning)
            return true;
        return face_frustum.intersects(section->world_bounds);
    }

}

ShadowPass::ShadowPass()
{
    m_type = RenderPass::Type::Shadow;
}

void ShadowPass::draw(RenderPassContext& context)
{
    if (m_render_directional_shadows)
        drawDirectionalLightShadowMap(context);
    if (m_render_point_shadows && !context.frameData().point_lights.empty())
        drawPointLightShadowMap(context);
}

void ShadowPass::drawDirectionalLightShadowMap(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(slot("outTarget"));
    if (!framebuffer)
        return;
    m_command_buffer->beginPass(framebuffer);

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::SingleColorShader));
    ShaderResourceBindings bindings;
    Mat4 light_view = context.frameData().directional_lights.front().lightViewMatrix;
    Mat4 light_proj = context.frameData().directional_lights.front().lightProjMatrix;
    for (const RenderMeshSection* render_node : context.renderScene().visibleMeshSections())
    {
        if (!render_node)
            continue;

        bindings.setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            bindings.setInt("bone_count", bone_count);
            bindings.setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            bindings.setInt("bone_count", 0);
        }
        bindings.setMatrix("model", 1, render_node->model_matrix);
        bindings.setMatrix("view", 1, light_view);
        bindings.setMatrix("projection", 1, light_proj);
        bindings.setFloat4("color", Color4(1.0));
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}

void ShadowPass::drawPointLightShadowMap(RenderPassContext& context)
{
    std::array<const RenderPointLightData*, MAX_CUBE_SHADOW_MAP_COUNT> shadow_lights{};
    size_t shadow_light_count = 0;
    for (const auto &point_light : context.frameData().point_lights)
    {
        if (point_light.shadow_index < 0 || point_light.shadow_index >= static_cast<int>(MAX_CUBE_SHADOW_MAP_COUNT))
            continue;
        shadow_lights[point_light.shadow_index] = &point_light;
        shadow_light_count = std::max(shadow_light_count, static_cast<size_t>(point_light.shadow_index + 1));
    }

    if (shadow_light_count == 0)
        return;

    context.ensureCubeShadowMapsCount(shadow_light_count);

    auto drawCasterList = [this](
                              const std::vector<RenderMeshSection*>& caster_sections,
                              const RenderPointLightData& light,
                              int face,
                              const RenderFrustum& face_frustum)
    {
        ShaderResourceBindings bindings;
        for (const RenderMeshSection* render_node : caster_sections)
        {
            if (!shouldDrawPointShadowCaster(render_node, light.position, light.radius, face_frustum))
                continue;

            bindings.setBool("useSkinning", render_node->use_skinning);
            if (render_node->use_skinning && !render_node->bone_matrices.empty())
            {
                int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
                bindings.setInt("bone_count", bone_count);
                bindings.setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
            }
            else
            {
                bindings.setInt("bone_count", 0);
            }
            bindings.setMatrix("model", 1, render_node->model_matrix);
            bindings.setMatrix("view", 1, light.lightViewMatrix[face]);
            bindings.setMatrix("projection", 1, light.lightProjMatrix);
            bindings.setFloat3("lightPos", light.position);
            bindings.setFloat("far_plane", light.radius);
            m_command_buffer->setShaderResources(&bindings);
            m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
            m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
        }
    };

    const uint64_t static_version = context.renderScene().shadowStaticVersion();

    for (size_t cube_map_id = 0; cube_map_id < shadow_light_count; cube_map_id++)
    {
        const RenderPointLightData* light = shadow_lights[cube_map_id];
        if (!light)
            continue;

        PointShadowCacheEntry& cache = m_point_shadow_cache[cube_map_id];
        const bool same_light =
            cache.light_id == light->id &&
            cache.radius == light->radius &&
            cache.position.x == light->position.x &&
            cache.position.y == light->position.y &&
            cache.position.z == light->position.z;
        if (!same_light || cache.static_version != static_version)
        {
            cache.light_id = light->id;
            cache.position = light->position;
            cache.radius = light->radius;
            cache.static_version = static_version;
            cache.face_valid.fill(false);
        }

        for (int face = 0; face < 6; face++)
        {
            RhiFrameBuffer* work_face_framebuffer = context.cubeShadowFaceFrameBufferOf(cube_map_id, face);
            RhiFrameBuffer* static_face_framebuffer = context.cubeShadowStaticFaceFrameBufferOf(cube_map_id, face);
            if (!work_face_framebuffer || !static_face_framebuffer)
                continue;

            const RenderFrustum face_frustum = RenderFrustum::fromViewProjection(light->lightProjMatrix * light->lightViewMatrix[face]);
            if (cache.static_face_framebuffers[face] != static_face_framebuffer)
            {
                cache.static_face_framebuffers[face] = static_face_framebuffer;
                cache.face_valid[face] = false;
            }

            if (!cache.face_valid[face])
            {
                m_command_buffer->beginPass(static_face_framebuffer, Color4(1.0f, 1.0f, 1.0f, 1.0f));
                m_command_buffer->setViewport(0, 0, static_cast<int>(static_face_framebuffer->pixelSize().x), static_cast<int>(static_face_framebuffer->pixelSize().y));
                m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::CubeMapShader));
                drawCasterList(context.renderScene().staticShadowCasterSections(), *light, face, face_frustum);
                m_command_buffer->endPass();
                cache.face_valid[face] = true;
            }

            m_command_buffer->blit(static_face_framebuffer, work_face_framebuffer, RhiTexture::Format::DEPTH);

            m_command_buffer->beginPass(work_face_framebuffer, Color4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0, false, false);
            m_command_buffer->setViewport(0, 0, static_cast<int>(work_face_framebuffer->pixelSize().x), static_cast<int>(work_face_framebuffer->pixelSize().y));
            m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::CubeMapShader));
            drawCasterList(context.renderScene().dynamicShadowCasterSections(), *light, face, face_frustum);
            m_command_buffer->endPass();
        }
    }
}
