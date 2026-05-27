#include "OpenGLCommandBuffer.hpp"

#include <cassert>
#include <cstdint>

void OpenGLCommandBuffer::beginPass(RhiFrameBuffer* render_target,
                                    const Color4& color_clear_value,
                                    float depth_clear_value,
                                    int stencil_clear_value,
                                    bool clear_color,
                                    bool clear_depth_stencil)
{
    assert(render_target && "beginPass requires a valid render target");
    assert(!insidePass() && "beginPass called while another pass is active");

    m_current_target = render_target;
    m_current_pipeline = nullptr;
    m_current_vertex_layout = nullptr;
    m_index_offset = 0;
    m_index_format = IndexUInt32;

    // RhiFrameBuffer::bind() already knows the render target size and sets a
    // matching viewport for OpenGL FBOs. Keeping that behavior here lets existing
    // framebuffer code keep working while pass execution moves into CommandBuffer.
    m_current_target->bind();

    GLbitfield clear_mask = 0;
    if (clear_color)
    {
        glClearColor(color_clear_value.x, color_clear_value.y, color_clear_value.z, color_clear_value.w);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        clear_mask |= GL_COLOR_BUFFER_BIT;
    }

    if (clear_depth_stencil)
    {
        glDepthMask(GL_TRUE);
        glClearDepth(depth_clear_value);
        glStencilMask(0xFF);
        glClearStencil(stencil_clear_value);
        clear_mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    }

    if (clear_mask != 0)
        glClear(clear_mask);
}

void OpenGLCommandBuffer::endPass()
{
    assert(insidePass() && "endPass called without beginPass");

    glBindVertexArray(0);
    glUseProgram(0);
    m_current_target->unBind();

    m_current_target = nullptr;
    m_current_pipeline = nullptr;
    m_current_vertex_layout = nullptr;
}

void OpenGLCommandBuffer::setGraphicsPipeline(RhiGraphicsPipeline* pipeline)
{
    assert(insidePass() && "setGraphicsPipeline must be called inside a render pass");
    assert(pipeline && "setGraphicsPipeline requires a valid pipeline");

    OpenGLGraphicsPipeline* gl_pipeline = static_cast<OpenGLGraphicsPipeline*>(pipeline);
    m_current_pipeline = gl_pipeline;
    m_current_pipeline->bind();
}

void OpenGLCommandBuffer::setShaderResources(RhiShaderResourceBindings* bindings)
{
    assert(insidePass() && "setShaderResources must be called inside a render pass");
    assert(m_current_pipeline && "setShaderResources requires a graphics pipeline");
    if (!bindings)
        return;

    const GL_HANDLE program = m_current_pipeline->program();
    for (const RhiShaderResourceBindings::Binding& binding : bindings->bindings())
    {
        const GLint location = glGetUniformLocation(program, binding.name.c_str());
        switch (binding.type)
        {
        case RhiShaderResourceBindings::Bool:
        case RhiShaderResourceBindings::Int:
            if (location != -1)
                glUniform1i(location, binding.int_value);
            break;
        case RhiShaderResourceBindings::Float:
            if (location != -1)
                glUniform1f(location, binding.values[0]);
            break;
        case RhiShaderResourceBindings::Float3:
            if (location != -1)
                glUniform3f(location, binding.values[0], binding.values[1], binding.values[2]);
            break;
        case RhiShaderResourceBindings::Float4:
            if (location != -1)
                glUniform4f(location, binding.values[0], binding.values[1], binding.values[2], binding.values[3]);
            break;
        case RhiShaderResourceBindings::Matrix:
            if (location != -1 && !binding.matrices.empty())
                glUniformMatrix4fv(location,
                                   static_cast<GLsizei>(binding.matrices.size()),
                                   GL_FALSE,
                                   &(binding.matrices[0][0].x));
            break;
        case RhiShaderResourceBindings::Texture2D:
            glActiveTexture(GL_TEXTURE0 + binding.texture_unit);
            glBindTexture(GL_TEXTURE_2D, binding.texture_id);
            if (location != -1)
                glUniform1i(location, binding.texture_unit);
            break;
        case RhiShaderResourceBindings::TextureCube:
            glActiveTexture(GL_TEXTURE0 + binding.texture_unit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, binding.texture_id);
            if (location != -1)
                glUniform1i(location, binding.texture_unit);
            break;
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

void OpenGLCommandBuffer::setVertexInput(RhiVertexLayout* layout,
                                         RhiBuffer* index_buffer,
                                         int index_offset,
                                         IndexFormat index_format)
{
    assert(insidePass() && "setVertexInput must be called inside a render pass");
    assert(layout && "setVertexInput requires a valid vertex layout");

    m_current_vertex_layout = layout;
    m_index_offset = index_offset;
    m_index_format = index_format;

    // 现有 OpenGLVertexLayout 已经把 vertex buffer 和默认 index buffer 捕获进 VAO。
    // 这里绑定 VAO 即可。如果调用方传入 index_buffer，则临时覆盖 VAO 中的
    // GL_ELEMENT_ARRAY_BUFFER，便于之后逐步迁移到“layout + buffers”分离模型。
    glBindVertexArray(layout->id());
    if (index_buffer)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->id());
}

void OpenGLCommandBuffer::setViewport(int x, int y, int width, int height)
{
    assert(insidePass() && "setViewport must be called inside a render pass");
    glViewport(x, y, width, height);
}

void OpenGLCommandBuffer::setScissor(int x, int y, int width, int height)
{
    assert(insidePass() && "setScissor must be called inside a render pass");

    // scissor 是动态状态。pipeline 用 UsesScissor 表示“这个 pipeline 期待外部
    // 提供 scissor”，CommandBuffer 在这里真正给出矩形。
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void OpenGLCommandBuffer::setBlendConstants(const Color4& c)
{
    assert(insidePass() && "setBlendConstants must be called inside a render pass");
    glBlendColor(c.x, c.y, c.z, c.w);
}

void OpenGLCommandBuffer::setStencilRef(int ref_value)
{
    assert(insidePass() && "setStencilRef must be called inside a render pass");
    if (m_current_pipeline)
        m_current_pipeline->applyStencilRef(ref_value);
}

void OpenGLCommandBuffer::blit(RhiFrameBuffer* source,
                               RhiFrameBuffer* dest,
                               RhiTexture::Format format)
{
    assert(source && "blit requires a valid source framebuffer");
    assert(dest && "blit requires a valid destination framebuffer");
    assert(!insidePass() && "blit must be issued outside beginPass/endPass");

    // blit 本质是一次 framebuffer-to-framebuffer copy，不依赖 graphics pipeline。
    // 放进 CommandBuffer 是为了让 RenderPass 不再直接调用 framebuffer->blitTo()，
    // 但底层仍复用现有 OpenGLFrameBuffer 的实现。
    source->blitTo(dest, format);
}

void OpenGLCommandBuffer::draw(int vertex_count,
                               int instance_count,
                               int first_vertex,
                               int first_instance)
{
    assert(insidePass() && "draw must be called inside a render pass");
    assert(m_current_pipeline && "draw requires a graphics pipeline");
    assert(m_current_vertex_layout && "draw requires vertex input");

    (void)first_instance; // OpenGL 4.3 支持 base instance，但第一版 RHI 先不暴露差异。

    if (instance_count <= 1)
    {
        glDrawArrays(m_current_pipeline->drawMode(), first_vertex, vertex_count);
    }
    else
    {
        glDrawArraysInstanced(m_current_pipeline->drawMode(), first_vertex, vertex_count, instance_count);
    }
}

void OpenGLCommandBuffer::drawIndexed(int index_count,
                                      int instance_count,
                                      int first_index,
                                      int vertex_offset,
                                      int first_instance)
{
    assert(insidePass() && "drawIndexed must be called inside a render pass");
    assert(m_current_pipeline && "drawIndexed requires a graphics pipeline");
    assert(m_current_vertex_layout && "drawIndexed requires vertex input");

    (void)first_instance;

    const GLenum index_type = m_index_format == IndexUInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    const void* index_pointer = reinterpret_cast<const void*>(
        static_cast<intptr_t>(m_index_offset + first_index * indexStride()));

    if (instance_count <= 1)
    {
        if (vertex_offset == 0)
        {
            glDrawElements(m_current_pipeline->drawMode(), index_count, index_type, index_pointer);
        }
        else
        {
            glDrawElementsBaseVertex(m_current_pipeline->drawMode(), index_count, index_type, index_pointer, vertex_offset);
        }
    }
    else
    {
        if (vertex_offset == 0)
        {
            glDrawElementsInstanced(m_current_pipeline->drawMode(), index_count, index_type, index_pointer, instance_count);
        }
        else
        {
            glDrawElementsInstancedBaseVertex(
                m_current_pipeline->drawMode(),
                index_count,
                index_type,
                index_pointer,
                instance_count,
                vertex_offset);
        }
    }
}
