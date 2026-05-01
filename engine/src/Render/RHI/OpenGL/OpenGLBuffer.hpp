#ifndef OpenGLBuffer_hpp
#define OpenGLBuffer_hpp

#include "Render/RHI/OpenGL/rhi_opengl.hpp"

class OpenGLBuffer : public RhiBuffer {
public:
	OpenGLBuffer(Type type_, UsageFlag usage_, void* data, int size_);
	bool create() override;
    void update(void* data, int size, int offset = 0) override;

private:
	GLenum m_target_enum;
};


#endif // !OpenGLBuffer
