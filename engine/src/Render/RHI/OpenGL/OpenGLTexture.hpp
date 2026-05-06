#ifndef OpenGLTexture_hpp
#define OpenGLTexture_hpp

#include "Render/RHI/OpenGL/rhi_opengl.hpp"

class OpenGLTexture : public RhiTexture {
public:
    OpenGLTexture(Format format_, const Vec2& pixelSize_, int sampleCount_, Flag flags_, unsigned char* data);
    bool create() override;
    void destroy() override;
};


#endif // !OpenGLTexture_hpp
