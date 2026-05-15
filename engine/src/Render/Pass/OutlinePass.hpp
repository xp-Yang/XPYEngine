#ifndef OutlinePass_hpp
#define OutlinePass_hpp

#include "RenderPass.hpp"

class OutlinePass : public RenderPass
{
public:
	OutlinePass();
	void draw(RenderPassContext& context) override;
};

#endif
