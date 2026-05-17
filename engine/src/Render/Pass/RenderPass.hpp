#ifndef RenderPass_hpp
#define RenderPass_hpp

#include "Render/RenderSourceData.hpp"

class RenderPassContext;

static inline constexpr float DEFAULT_RENDER_RESOLUTION_X = 1920.0f;
static inline constexpr float DEFAULT_RENDER_RESOLUTION_Y = 1080.0f;
// for debug visualization
static inline int PickingColorIDFactor = 256 * 256 * 256 / 500;

// Interface class
// A RenderPass describes draw work; RenderGraph owns the render targets it writes to.

//// 旧设计：
//// each RenderPass corresponds to a framebuffer
//// there could be different attachments(render targets, like texture/render buffer object) in one framebuffer
//// perhaps we need subpass, so that can use input attachments to render
//// a subpass (now as a RenderPass) may contain multiple graphics-pipeline, and execute graphics-pipeline multiple times
//// a graphics-pipeline need shader program and vertices to execute

class RenderPass {
public:
	enum class Type {
        Unknown,
        
        ZPre,
        Picking,
        Outline,
        SkyBox,
        Shadow,
        Forward,
        GBuffer,
        DeferredLighting,
        Transparent,
        
        WireFrame,
        CheckerBoard,
        Normal,
        RayTracing,
        
        // post process
        Bloom,
        FXAA,
        ColorGrading,
        
        Final,
	};

	RenderPass() : m_rhi(RenderSourceData::rhi) {}
	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;
	virtual ~RenderPass() = default;
	virtual void draw(RenderPassContext& context) = 0;

protected:
	std::shared_ptr<Rhi> m_rhi;

	Type m_type;
};

#endif // !RenderPass_hpp
