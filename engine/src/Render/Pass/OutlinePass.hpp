#ifndef OutlinePass_hpp
#define OutlinePass_hpp

#include "RenderPass.hpp"

class OutlinePass : public RenderPass
{
public:
	OutlinePass();
	void draw() override;
	void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
	void init() override;

private:
	void rebuildFramebuffer(const Vec2& pixel_size);
	std::unique_ptr<RhiFrameBuffer> m_source_framebuffer;
};

#endif
