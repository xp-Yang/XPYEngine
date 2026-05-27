#include "RenderPipelineLibrary.hpp"

#include <memory>

namespace
{
struct PipelineVariant
{
    ShaderType shader_type{ ShaderType::None };
    RenderPipelineState state;
    RhiVertexLayout* vertex_layout{ nullptr };
    std::unique_ptr<RhiGraphicsPipeline> pipeline;
};

std::vector<PipelineVariant>& pipelineCache()
{
    static std::vector<PipelineVariant> cache;
    return cache;
}

std::vector<RhiShaderStage> shaderStagesOf(const ShaderType& shader_type, const Shader& shader)
{
    std::vector<RhiShaderStage> stages;
    stages.emplace_back(RhiShaderStage::Vertex, shader.vsCode, "ShaderType " + std::to_string(static_cast<int>(shader_type)) + " vertex");
    stages.emplace_back(RhiShaderStage::Fragment, shader.fsCode, "ShaderType " + std::to_string(static_cast<int>(shader_type)) + " fragment");
    if (!shader.gsCode.empty())
        stages.emplace_back(RhiShaderStage::Geometry, shader.gsCode, "ShaderType " + std::to_string(static_cast<int>(shader_type)) + " geometry");
    return stages;
}
}

RhiGraphicsPipeline* RenderPipelineLibrary::graphicsPipeline(const ShaderType& shader_type,
                                                             const RenderPipelineState& state,
                                                             RhiVertexLayout* vertex_layout)
{
    std::vector<PipelineVariant>& cache = pipelineCache();
    for (PipelineVariant& variant : cache)
    {
        if (variant.shader_type == shader_type &&
            variant.state == state &&
            variant.vertex_layout == vertex_layout)
        {
            return variant.pipeline.get();
        }
    }

    const Shader& shader = Shader::get(shader_type);
    std::vector<RhiShaderStage> stages = shaderStagesOf(shader_type, shader);

    std::unique_ptr<RhiGraphicsPipeline> pipeline(Rhi::get()->newGraphicsPipeline());
    pipeline->setShaderStages(stages.begin(), stages.end());
    pipeline->setTopology(state.topology);
    pipeline->setCullMode(state.cullMode);
    pipeline->setFrontFace(state.frontFace);
    pipeline->setDepthTest(state.depthTest);
    pipeline->setDepthWrite(state.depthWrite);
    pipeline->setVertexInputLayout(vertex_layout);

    if (state.blend)
    {
        RhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = RhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = RhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = RhiGraphicsPipeline::SrcAlpha;
        blend.dstAlpha = RhiGraphicsPipeline::OneMinusSrcAlpha;
        pipeline->setTargetBlends({ blend });
    }

    if (!pipeline->create())
    {
        Logger::error("RenderPipelineLibrary failed to create graphics pipeline.");
        assert(false);
        return nullptr;
    }

    RhiGraphicsPipeline* result = pipeline.get();
    cache.push_back(PipelineVariant{ shader_type, state, vertex_layout, std::move(pipeline) });
    return result;
}

void RenderPipelineLibrary::clear()
{
    pipelineCache().clear();
}
