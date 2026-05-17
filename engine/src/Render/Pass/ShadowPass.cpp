#include "ShadowPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"
#include <algorithm>

ShadowPass::ShadowPass()
{
    m_type = RenderPass::Type::Shadow;
}

void ShadowPass::draw(RenderPassContext& context)
{
    drawDirectionalLightShadowMap(context);
    drawPointLightShadowMap(context);
}

void ShadowPass::drawDirectionalLightShadowMap(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    if (!framebuffer)
        return;
    framebuffer->bind();
    framebuffer->clear();

    static RenderShaderObject *depth_shader = RenderShaderObject::getShaderObject(ShaderType::SingleColorShader);
    depth_shader->start_using();
    Mat4 light_view = context.renderSourceData().render_directional_light_data_list.front().lightViewMatrix;
    Mat4 light_proj = context.renderSourceData().render_directional_light_data_list.front().lightProjMatrix;
    for (const auto &pair : context.renderSourceData().render_mesh_nodes)
    {
        const auto &render_node = pair.second;
        depth_shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            depth_shader->setInt("bone_count", bone_count);
            depth_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            depth_shader->setInt("bone_count", 0);
        }
        depth_shader->setMatrix("model", 1, render_node->model_matrix);
        depth_shader->setMatrix("view", 1, light_view);
        depth_shader->setMatrix("projection", 1, light_proj);
        depth_shader->setFloat4("color", Color4(1.0));
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
}

void ShadowPass::drawPointLightShadowMap(RenderPassContext& context)
{
    static RenderShaderObject *depth_shader = RenderShaderObject::getShaderObject(ShaderType::CubeMapShader);

    depth_shader->start_using();

    std::vector<std::array<Mat4, 6>> light_view;
    std::vector<Mat4> light_proj;
    std::vector<Vec3> light_pos;
    std::vector<float> light_radius;
    for (const auto &render_point_light_data : context.renderSourceData().render_point_light_data_list)
    {
        light_view.push_back(render_point_light_data.lightViewMatrix);
        light_proj.push_back(render_point_light_data.lightProjMatrix);
        light_pos.push_back(render_point_light_data.position);
        light_radius.push_back(render_point_light_data.radius);
    }

    context.ensureCubeDepthTextureCount(context.renderSourceData().render_point_light_data_list.size());
    const std::vector<unsigned int>& cube_maps = context.cubeDepthTextures();
    const unsigned int cube_framebuffer = context.cubeDepthFrameBuffer();
    const int cube_edge = context.cubeDepthEdge();
    if (cube_framebuffer == 0 || cube_edge <= 0 || cube_maps.empty())
        return;

    m_rhi->bindFramebuffer(cube_framebuffer);
    m_rhi->setViewport(0, 0, cube_edge, cube_edge);

    for (int cube_map_id = 0; cube_map_id < context.renderSourceData().render_point_light_data_list.size(); cube_map_id++)
    {
        for (int i = 0; i < 6; i++)
        {
            m_rhi->attachDepthCubeFace(cube_maps[cube_map_id], i);
            m_rhi->clearColorDepthStencil(1.0f, 1.0f, 1.0f, 1.0f);

            for (const auto &pair : context.renderSourceData().render_mesh_nodes)
            {
                const auto &render_node = pair.second;
                depth_shader->setBool("useSkinning", render_node->use_skinning);
                if (render_node->use_skinning && !render_node->bone_matrices.empty())
                {
                    int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
                    depth_shader->setInt("bone_count", bone_count);
                    depth_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
                }
                else
                {
                    depth_shader->setInt("bone_count", 0);
                }
                depth_shader->setMatrix("model", 1, render_node->model_matrix);
                depth_shader->setMatrix("view", 1, light_view[cube_map_id][i]);
                depth_shader->setMatrix("projection", 1, light_proj[cube_map_id]);
                depth_shader->setFloat3("lightPos", light_pos[cube_map_id]);
                depth_shader->setFloat("far_plane", light_radius[cube_map_id]);
                m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
            }
        }
    }
}
