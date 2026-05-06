#ifndef CheckerBoardPass_hpp
#define CheckerBoardPass_hpp

#include "RenderPass.hpp"

class CheckerBoardPass : public RenderPass
{
public:
	CheckerBoardPass();
	void draw() override;
	void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
	void init() override;

private:
	void rebuildFramebuffer(const Vec2& pixel_size);
};

#endif