#ifndef BloomPass_hpp
#define BloomPass_hpp

#include "RenderPass.hpp"

class BloomPass : public RenderPass
{
public:
	BloomPass();
	void draw() override;
	void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
	void init() override;
	void extractBright();
	void blur();

private:
	void rebuildFramebuffer(const Vec2& pixel_size);

	std::unique_ptr<RhiFrameBuffer> m_pingpong_framebuffer;
};

#endif