#ifndef OpenGLVertexLayout_hpp
#define OpenGLVertexLayout_hpp

#include "Render/RHI/OpenGL/rhi_opengl.hpp"

class OpenGLVertexLayout : public RhiVertexLayout {
public:
	OpenGLVertexLayout(RhiBuffer* vbuffer, RhiBuffer* ibuffer);
	bool create() override;
	bool createInstancing(RhiBuffer* inst_buffer, std::initializer_list<RhiVertexAttribute> attributes) override;
};


#endif // !OpenGLVertexLayout
