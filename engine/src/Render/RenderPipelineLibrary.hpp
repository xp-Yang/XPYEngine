#ifndef RenderPipelineLibrary_hpp
#define RenderPipelineLibrary_hpp

#include "Logical/Shader.hpp"
#include "Render/RHI/rhi.hpp"

struct RenderPipelineState {
    bool blend{ false };
    bool depthTest{ true };
    bool depthWrite{ true };
    RhiGraphicsPipeline::CullMode cullMode{ RhiGraphicsPipeline::Back };
    RhiGraphicsPipeline::FrontFace frontFace{ RhiGraphicsPipeline::CCW };
    RhiGraphicsPipeline::Topology topology{ RhiGraphicsPipeline::Triangles };

    bool operator==(const RenderPipelineState& rhs) const
    {
        return blend == rhs.blend &&
            depthTest == rhs.depthTest &&
            depthWrite == rhs.depthWrite &&
            cullMode == rhs.cullMode &&
            frontFace == rhs.frontFace &&
            topology == rhs.topology;
    }
};

class RenderPipelineLibrary {
public:
    static RhiGraphicsPipeline* graphicsPipeline(const ShaderType& shader_type,
                                                 const RenderPipelineState& state = RenderPipelineState(),
                                                 RhiVertexLayout* vertex_layout = nullptr);
    static void clear();

private:
    RenderPipelineLibrary() = delete;
};

#endif // !RenderPipelineLibrary_hpp
