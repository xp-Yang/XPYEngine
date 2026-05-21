#include "OpenGLRenderer.hpp"

void OpenGLRenderer::drawIndexed(GL_HANDLE vao_id, size_t indices_count, size_t index_offset, int inst_amount)
{
    glBindVertexArray(vao_id);
    void* index_buffer_offset = reinterpret_cast<void*>(index_offset * sizeof(GLuint));
    if (inst_amount == -1)
        glDrawElements(GL_TRIANGLES, (int)indices_count, GL_UNSIGNED_INT, index_buffer_offset);
    else
        glDrawElementsInstanced(GL_TRIANGLES, (int)indices_count, GL_UNSIGNED_INT, index_buffer_offset, inst_amount);
    glBindVertexArray(0);
}

void OpenGLRenderer::drawTriangle(GL_HANDLE vao_id, size_t array_count)
{
    glBindVertexArray(vao_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)array_count);
    glBindVertexArray(0);
}
