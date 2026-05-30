#include "ShadowPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"
#include <algorithm>

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
    std::vector<std::array<Mat4, 6>> light_view;
    std::vector<Mat4> light_proj;
    std::vector<Vec3> light_pos;
    std::vector<float> light_radius;
    for (const auto &render_point_light_data : context.frameData().point_lights)
    {
        light_view.push_back(render_point_light_data.lightViewMatrix);
        light_proj.push_back(render_point_light_data.lightProjMatrix);
        light_pos.push_back(render_point_light_data.position);
        light_radius.push_back(render_point_light_data.radius);
    }

    context.ensureCubeShadowMapsCount(std::min(MAX_CUBE_SHADOW_MAP_COUNT, context.frameData().point_lights.size()));

    for (int cube_map_id = 0; cube_map_id < context.frameData().point_lights.size(); cube_map_id++)
    {
        for (int i = 0; i < 6; i++)
        {
            RhiFrameBuffer* face_framebuffer = context.cubeShadowFaceFrameBufferOf(cube_map_id, i);

            if (!face_framebuffer)
                continue;
            m_command_buffer->beginPass(face_framebuffer, Color4(1.0f, 1.0f, 1.0f, 1.0f));
            m_command_buffer->setViewport(0, 0, static_cast<int>(face_framebuffer->pixelSize().x), static_cast<int>(face_framebuffer->pixelSize().y));
            m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::CubeMapShader));
            ShaderResourceBindings bindings;

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
                bindings.setMatrix("view", 1, light_view[cube_map_id][i]);
                bindings.setMatrix("projection", 1, light_proj[cube_map_id]);
                bindings.setFloat3("lightPos", light_pos[cube_map_id]);
                bindings.setFloat("far_plane", light_radius[cube_map_id]);
                m_command_buffer->setShaderResources(&bindings);
                m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
                m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
            }
            m_command_buffer->endPass();
        }
    }
}
