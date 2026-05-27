#ifndef OpenGLCommandBuffer_hpp
#define OpenGLCommandBuffer_hpp

#include "Render/RHI/OpenGL/OpenGLGraphicsPipeline.hpp"

// OpenGL-only 的轻量 CommandBuffer。
//
// 这个类有意保持 Qt RHI 的调用形态，但当前不做真正的命令延迟录制：
// 每个函数会立即执行对应的 OpenGL 调用。这样可以先把上层 RenderPass 从
// “直接操作 OpenGL 状态”迁移到“通过 RHI 命令描述渲染工作”，同时避免一次性
// 引入完整 command list、resource barrier、frame submission 等复杂系统。
class OpenGLCommandBuffer : public RhiCommandBuffer {
public:
    OpenGLCommandBuffer() = default;

    void beginPass(RhiFrameBuffer* render_target,
                   const Color4& color_clear_value = Color4(0.f, 0.f, 0.f, 1.f),
                   float depth_clear_value = 1.0f,
                   int stencil_clear_value = 0,
                   bool clear_color = true,
                   bool clear_depth_stencil = true) override;
    void endPass() override;

    void setGraphicsPipeline(RhiGraphicsPipeline* pipeline) override;
    void setShaderResources(RhiShaderResourceBindings* bindings = nullptr) override;
    void setVertexInput(RhiVertexLayout* layout,
                        RhiBuffer* index_buffer = nullptr,
                        int index_offset = 0,
                        IndexFormat index_format = IndexUInt32) override;
    void setViewport(int x, int y, int width, int height) override;
    void setScissor(int x, int y, int width, int height) override;
    void setBlendConstants(const Color4& c) override;
    void setStencilRef(int ref_value) override;
    void blit(RhiFrameBuffer* source,
              RhiFrameBuffer* dest,
              RhiTexture::Format format = RhiTexture::Format::RGBA16F) override;

    void draw(int vertex_count,
              int instance_count = 1,
              int first_vertex = 0,
              int first_instance = 0) override;
    void drawIndexed(int index_count,
                     int instance_count = 1,
                     int first_index = 0,
                     int vertex_offset = 0,
                     int first_instance = 0) override;

private:
    bool insidePass() const { return m_current_target != nullptr; }
    int indexStride() const { return m_index_format == IndexUInt16 ? 2 : 4; }

    RhiFrameBuffer* m_current_target{ nullptr };
    OpenGLGraphicsPipeline* m_current_pipeline{ nullptr };
    RhiVertexLayout* m_current_vertex_layout{ nullptr };
    int m_index_offset{ 0 };
    IndexFormat m_index_format{ IndexUInt32 };
};

#endif // !OpenGLCommandBuffer_hpp
