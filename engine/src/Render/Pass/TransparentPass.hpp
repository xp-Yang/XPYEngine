#ifndef TransparentPass_hpp
#define TransparentPass_hpp

#include "RenderPass.hpp"

class TransparentPass : public RenderPass {
public:
    TransparentPass();
    void draw(RenderPassContext& context) override;

protected:
    void init();
};

#endif // !TransparentPass_hpp
