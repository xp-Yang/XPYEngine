#ifndef CheckerBoardPass_hpp
#define CheckerBoardPass_hpp

#include "RenderPass.hpp"

class CheckerBoardPass : public RenderPass
{
public:
	CheckerBoardPass();
	void draw(RenderPassContext& context) override;
};

#endif
