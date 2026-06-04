#include "OpenGLVertexLayout.hpp"

OpenGLVertexLayout::OpenGLVertexLayout(RhiBuffer* vbuffer, RhiBuffer* ibuffer)
    : RhiVertexLayout(vbuffer, ibuffer)
{
}

OpenGLVertexLayout::~OpenGLVertexLayout()
{
    destroy();
}

bool OpenGLVertexLayout::create()
{
    glGenVertexArrays(1, &m_id);
    glBindVertexArray(m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbuffer->id());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibuffer->id());
    for (size_t i = 0; i < m_attributes.size(); ++i) {
        int location = m_attributes[i].location;
        if (location == -1)
            continue;
        int stride = m_attributes[i].stride;
        int offset = m_attributes[i].offset;
        int size = 1;
        GLenum type = GL_FLOAT;
        bool integer_attrib = false;
        switch (m_attributes[i].format) {
        case RhiVertexAttribute::Float4:
            type = GL_FLOAT;
            size = 4;
            break;
        case RhiVertexAttribute::Float3:
            type = GL_FLOAT;
            size = 3;
            break;
        case RhiVertexAttribute::Float2:
            type = GL_FLOAT;
            size = 2;
            break;
        case RhiVertexAttribute::Float:
            type = GL_FLOAT;
            size = 1;
            break;
        case RhiVertexAttribute::UInt4:
            type = GL_UNSIGNED_INT;
            size = 4;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::UInt3:
            type = GL_UNSIGNED_INT;
            size = 3;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::UInt2:
            type = GL_UNSIGNED_INT;
            size = 2;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::UInt:
            type = GL_UNSIGNED_INT;
            size = 1;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt4:
            type = GL_INT;
            size = 4;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt3:
            type = GL_INT;
            size = 3;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt2:
            type = GL_INT;
            size = 2;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt:
            type = GL_INT;
            size = 1;
            integer_attrib = true;
            break;
        default:
            break;
        }
        glEnableVertexAttribArray(location);
        if (integer_attrib)
            glVertexAttribIPointer(location, size, type, stride, (GLvoid *)(offset));
        else
            glVertexAttribPointer(location, size, type, false, stride, (GLvoid *)(offset));
    }
    return true;
}

void OpenGLVertexLayout::destroy()
{
    if (m_id != 0)
    {
        glDeleteVertexArrays(1, &m_id);
        m_id = 0;
    }
}

bool OpenGLVertexLayout::createInstancing(RhiBuffer* inst_buffer, std::initializer_list<RhiVertexAttribute> attributes)
{
    glBindVertexArray(m_id);
    glBindBuffer(GL_ARRAY_BUFFER, inst_buffer->id());
    for (const RhiVertexAttribute& attribute : attributes) {
        int location = attribute.location;
        if (location == -1)
            continue;
        int stride = attribute.stride;
        int offset = attribute.offset;
        int size = 1;
        GLenum type = GL_FLOAT;
        bool integer_attrib = false;
        switch (attribute.format) {
        case RhiVertexAttribute::Float4:
            type = GL_FLOAT;
            size = 4;
            break;
        case RhiVertexAttribute::Float3:
            type = GL_FLOAT;
            size = 3;
            break;
        case RhiVertexAttribute::Float2:
            type = GL_FLOAT;
            size = 2;
            break;
        case RhiVertexAttribute::Float:
            type = GL_FLOAT;
            size = 1;
            break;
        case RhiVertexAttribute::UInt4:
            type = GL_UNSIGNED_INT;
            size = 4;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::UInt3:
            type = GL_UNSIGNED_INT;
            size = 3;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::UInt2:
            type = GL_UNSIGNED_INT;
            size = 2;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::UInt:
            type = GL_UNSIGNED_INT;
            size = 1;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt4:
            type = GL_INT;
            size = 4;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt3:
            type = GL_INT;
            size = 3;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt2:
            type = GL_INT;
            size = 2;
            integer_attrib = true;
            break;
        case RhiVertexAttribute::SInt:
            type = GL_INT;
            size = 1;
            integer_attrib = true;
            break;
        default:
            break;
        }
        glEnableVertexAttribArray(location);
        if (integer_attrib)
            glVertexAttribIPointer(location, size, type, stride, (GLvoid *)(offset));
        else
            glVertexAttribPointer(location, size, type, false, stride, (GLvoid *)(offset));
        glVertexAttribDivisor(location, 1);
    }
    return true;
}
