#include "GBufferPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

GBufferPass::GBufferPass()
{
    m_type = RenderPass::Type::GBuffer;
}

void GBufferPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    if (!framebuffer)
        return;
    m_command_buffer->beginPass(framebuffer);

    const ShaderType shader_type = m_pbr ? ShaderType::GBufferShader : ShaderType::GBufferPhongShader;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(shader_type));
    ShaderResourceBindings bindings;
    bindings.setMatrix("view", 1, context.frameData().view_matrix);
    bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
    for (const RenderMeshSection* render_node : context.renderScene().opaqueMeshSections()) {
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
        const auto& material = render_node->material;
        if (m_pbr) {
            bindings.setFloat3("base_color_factor", material.base_color_factor);
            bindings.setFloat("metallic_factor", material.metallic_factor);
            bindings.setFloat("roughness_factor", material.roughness_factor);
            bindings.setFloat("ao_factor", material.ao_factor);
            bindings.setTexture("albedo_map", 0, material.albedo_map);
            bindings.setTexture("metallic_map", 1, material.metallic_map);
            bindings.setTexture("roughness_map", 2, material.roughness_map);
            bindings.setTexture("ao_map", 3, material.ao_map);
        }
        else {
            bindings.setFloat3("diffuse_factor", material.diffuse_factor);
            bindings.setFloat3("specular_factor", material.specular_factor);
            bindings.setFloat("shininess", material.shininess);
            bindings.setTexture("diffuse_map", 0, material.diffuse_map);
            bindings.setTexture("specular_map", 1, material.specular_map);
        }
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}

void GBufferPass::enablePBR(bool enable)
{
    m_pbr = enable;
}
