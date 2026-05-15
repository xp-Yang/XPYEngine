#ifndef ZPrePass_hpp
#define ZPrePass_hpp

#include "RenderPass.hpp"

class ZPrePass : public RenderPass {
public:
    ZPrePass();
    void draw(RenderPassContext& context) override;
};

#endif // !ZPrePass_hpp
