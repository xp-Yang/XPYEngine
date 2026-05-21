#ifndef OpenGLGraphicsPipeline_hpp
#define OpenGLGraphicsPipeline_hpp

#include "Render/RHI/OpenGL/rhi_opengl.hpp"

// OpenGL 版本的 GraphicsPipeline。
//
// 在 Vulkan/Metal 里 pipeline 通常是真正的 native pipeline object；OpenGL 没有
// 完全等价物，所以这里用一个类把“shader program + 固定渲染状态”组合起来。
// setGraphicsPipeline() 时，CommandBuffer 会调用 bind()，统一应用 program、
// cull/depth/blend/stencil/topology 等状态。
class OpenGLGraphicsPipeline : public RhiGraphicsPipeline {
public:
    OpenGLGraphicsPipeline() = default;
    ~OpenGLGraphicsPipeline() override;

    bool create() override;
    void destroy();

    GL_HANDLE id() const override { return m_program; }
    GL_HANDLE program() const { return m_program; }
    GLenum drawMode() const { return m_draw_mode; }

    // 绑定 pipeline 并把 RHI 描述转换成 OpenGL 状态。
    // 这是“固定状态集中化”的关键：pass 不再到处手动开关 GL_BLEND/GL_DEPTH_TEST。
    void bind();

    // stencil reference 是动态状态，因此不放进 create() 时的固定状态里。
    // pipeline 仍负责知道 stencil compare/mask/op，CommandBuffer 只提供 ref 值。
    void applyStencilRef(int ref_value);

private:
    GL_HANDLE m_program{ 0 };
    GLenum m_draw_mode{ GL_TRIANGLES };
    int m_stencil_ref{ 0 };
};

#endif // !OpenGLGraphicsPipeline_hpp
