#ifndef FinalPass_hpp
#define FinalPass_hpp

#include "RenderPass.hpp"

class FinalPass : public RenderPass
{
public:
    FinalPass();
    void draw(RenderPassContext& context) override;
    void setDrawGrid(bool enable) { m_draw_grid = enable; }

private:
    bool m_draw_grid = true;
};

#endif
