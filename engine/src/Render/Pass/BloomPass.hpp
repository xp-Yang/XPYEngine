#ifndef BloomPass_hpp
#define BloomPass_hpp

#include "RenderPass.hpp"

class BloomPass : public RenderPass
{
public:
	BloomPass();
	void draw(RenderPassContext& context) override;

protected:
	void extractBright(RenderPassContext& context);
	void blur(RenderPassContext& context);
	void writeToScene(RenderPassContext& context);
};

#endif
