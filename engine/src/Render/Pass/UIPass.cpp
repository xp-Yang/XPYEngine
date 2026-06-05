#include "UIPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

UIPass::UIPass()
{
    m_type = RenderPass::Type::UI;
}

void UIPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* default_framebuffer = context.defaultFrameBuffer();
    if (!default_framebuffer)
        return;

    m_command_buffer->beginPass(default_framebuffer, Color4(0.45f, 0.55f, 0.60f, 1.00f));
    m_command_buffer->endPass();
}
