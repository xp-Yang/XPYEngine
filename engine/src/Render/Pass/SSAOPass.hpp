#ifndef SSAOPass_hpp
#define SSAOPass_hpp

#include "RenderPass.hpp"
#include "Render/RenderParams.hpp"

#include <memory>
#include <vector>

// 屏幕空间环境光遮蔽：复用 GBuffer 世界空间 position/normal 计算半球采样遮蔽，
// 内部两段式（计算 -> 盒式模糊），最终结果写入图资源 SSAO.Result（R8）。
class SSAOPass : public RenderPass
{
public:
    SSAOPass();
    ~SSAOPass() override;

    void setParams(const SSAOParams& params) { m_params = params; }
    void draw(RenderPassContext& context) override;

private:
    void ensureRawBuffer(const Vec2& size);
    void destroyRawBuffer();
    void generateKernel();

    SSAOParams m_params;
    std::vector<Vec3> m_kernel;

    Vec2 m_current_size{ 0.f, 0.f };
    RhiTexture* m_raw_texture{ nullptr };
    std::unique_ptr<RhiFrameBuffer> m_raw_framebuffer;
};

#endif
