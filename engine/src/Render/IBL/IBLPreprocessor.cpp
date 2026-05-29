#include "IBLPreprocessor.hpp"

#include "Render/RHI/OpenGL/rhi_opengl.hpp" // glad/glad.h
#include "Base/Logger/Logger.hpp"

#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <cmath>

namespace {

// ---------------------------------------------------------------------------
// 内联着色器源码（标准 LearnOpenGL Split-Sum 流程）
// ---------------------------------------------------------------------------

const char* kCubeVS = R"(#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 localPos;
uniform mat4 projection;
uniform mat4 view;
void main() {
    localPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* kEquirectFS = R"(#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform sampler2D equirectangularMap;
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
void main() {
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
)";

const char* kIrradianceFS = R"(#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube environmentMap;
const float PI = 3.14159265359;
void main() {
    vec3 N = normalize(localPos);
    vec3 irradiance = vec3(0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);
    FragColor = vec4(irradiance, 1.0);
}
)";

const char* kPrefilterFS = R"(#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube environmentMap;
uniform float roughness;
uniform float resolution; // 源 cubemap 面分辨率
const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}
void main() {
    vec3 N = normalize(localPos);
    vec3 R = N;
    vec3 V = R;
    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float D = DistributionGGX(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;
            float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;
    FragColor = vec4(prefilteredColor, 1.0);
}
)";

const char* kBrdfVS = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
out vec2 UV;
void main() {
    UV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* kBrdfFS = R"(#version 330 core
out vec2 FragColor;
in vec2 UV;
const float PI = 3.14159265359;
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float k = (roughness * roughness) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}
vec2 IntegrateBRDF(float NdotV, float roughness) {
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    float A = 0.0;
    float B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if (NdotL > 0.0) {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    return vec2(A, B) / float(SAMPLE_COUNT);
}
void main() {
    FragColor = IntegrateBRDF(UV.x, UV.y);
}
)";

// ---------------------------------------------------------------------------
// 裸 GL 辅助
// ---------------------------------------------------------------------------

GLuint compileShader(GLenum type, const char* src)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        Logger::error("IBL shader compile failed: {}", static_cast<const char*>(log));
        glDeleteShader(id);
        return 0;
    }
    return id;
}

GLuint linkProgram(const char* vs_src, const char* fs_src)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        Logger::error("IBL program link failed: {}", static_cast<const char*>(log));
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

void setMat4(GLuint prog, const char* name, const glm::mat4& m)
{
    glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(m));
}
void setInt(GLuint prog, const char* name, int v)
{
    glUniform1i(glGetUniformLocation(prog, name), v);
}
void setFloat(GLuint prog, const char* name, float v)
{
    glUniform1f(glGetUniformLocation(prog, name), v);
}

// 单位立方体（位置）与全屏四边形（位置 + uv）的裸 GL 几何。
struct CaptureGeometry {
    GLuint cube_vao{ 0 }, cube_vbo{ 0 };
    GLuint quad_vao{ 0 }, quad_vbo{ 0 };

    void create()
    {
        static const float cube_vertices[] = {
            -1, -1, -1,  1,  1, -1,  1, -1, -1,  1,  1, -1, -1, -1, -1, -1,  1, -1,
            -1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1, -1,  1,
            -1,  1,  1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1,  1,  1,
             1,  1,  1,  1, -1, -1,  1,  1, -1,  1, -1, -1,  1,  1,  1,  1, -1,  1,
            -1, -1, -1,  1, -1, -1,  1, -1,  1,  1, -1,  1, -1, -1,  1, -1, -1, -1,
            -1,  1, -1,  1,  1,  1,  1,  1, -1,  1,  1,  1, -1,  1, -1, -1,  1,  1,
        };
        glGenVertexArrays(1, &cube_vao);
        glGenBuffers(1, &cube_vbo);
        glBindVertexArray(cube_vao);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        static const float quad_vertices[] = {
            // pos      // uv
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quad_vao);
        glGenBuffers(1, &quad_vbo);
        glBindVertexArray(quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }

    void drawCube() const
    {
        glBindVertexArray(cube_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    void drawQuad() const
    {
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
    void destroy()
    {
        glDeleteVertexArrays(1, &cube_vao);
        glDeleteBuffers(1, &cube_vbo);
        glDeleteVertexArrays(1, &quad_vao);
        glDeleteBuffers(1, &quad_vbo);
    }
};

const glm::mat4 kCaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
const std::array<glm::mat4, 6> kCaptureViews = {
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0)),
};

RhiTexture* makeCube(RhiTexture::Format format, int size, bool mipmapped)
{
    RhiTexture::Flag flags = mipmapped
        ? static_cast<RhiTexture::Flag>(RhiTexture::CubeMap | RhiTexture::MipMapped)
        : RhiTexture::CubeMap;
    std::array<unsigned char*, 6> empty{};
    RhiTexture* tex = Rhi::get()->newCubeTexture(format, Vec2((float)size, (float)size), 1, flags, empty);
    tex->create();
    return tex;
}

} // namespace

bool IBLPreprocessor::buildFromEquirectHDR(const std::string& hdr_path, IBLResources& out)
{
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channels = 0;
    float* data = stbi_loadf(hdr_path.c_str(), &width, &height, &channels, 3);
    stbi_set_flip_vertically_on_load(false);
    if (!data) {
        Logger::warn("IBL: failed to load HDR '{}', falling back to skybox cube.", hdr_path);
        return false;
    }

    GLuint hdr_tex = 0;
    glGenTextures(1, &hdr_tex);
    glBindTexture(GL_TEXTURE_2D, hdr_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    const int env_size = 512;
    RhiTexture* env_cube = makeCube(RhiTexture::Format::RGB16F, env_size, true);

    GLuint program = linkProgram(kCubeVS, kEquirectFS);
    if (!program) {
        glDeleteTextures(1, &hdr_tex);
        delete env_cube;
        return false;
    }

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);

    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    GLint prev_viewport[4];
    glGetIntegerv(GL_VIEWPORT, prev_viewport);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    CaptureGeometry geo;
    geo.create();

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glUseProgram(program);
    setInt(program, "equirectangularMap", 0);
    setMat4(program, "projection", kCaptureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr_tex);
    glViewport(0, 0, env_size, env_size);
    for (int face = 0; face < 6; ++face) {
        setMat4(program, "view", kCaptureViews[face]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, env_cube->id(), 0);
        glClear(GL_COLOR_BUFFER_BIT);
        geo.drawCube();
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cube->id());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    geo.destroy();
    glDeleteProgram(program);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &hdr_tex);

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    glEnable(GL_DEPTH_TEST);

    out.environment_cube = env_cube;
    out.owns_environment = true;
    return deriveFromEnvironment(out);
}

bool IBLPreprocessor::buildFromEnvironmentCube(RhiTexture* environment_cube, IBLResources& out)
{
    if (!environment_cube)
        return false;
    out.environment_cube = environment_cube;
    out.owns_environment = false;
    return deriveFromEnvironment(out);
}

bool IBLPreprocessor::deriveFromEnvironment(IBLResources& out)
{
    if (!out.environment_cube)
        return false;

    const int irradiance_size = 32;
    const int prefilter_size = 128;
    const int prefilter_mips = 5;
    const int brdf_size = 512;
    const float env_resolution = 512.0f;

    RhiTexture* irradiance = makeCube(RhiTexture::Format::RGB16F, irradiance_size, false);
    RhiTexture* prefilter = makeCube(RhiTexture::Format::RGB16F, prefilter_size, true);
    RhiTexture* brdf = Rhi::get()->newTexture(RhiTexture::Format::RGBA16F, Vec2((float)brdf_size, (float)brdf_size), 1, {}, nullptr);
    brdf->create();
    glBindTexture(GL_TEXTURE_2D, brdf->id());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint irradiance_prog = linkProgram(kCubeVS, kIrradianceFS);
    GLuint prefilter_prog = linkProgram(kCubeVS, kPrefilterFS);
    GLuint brdf_prog = linkProgram(kBrdfVS, kBrdfFS);
    if (!irradiance_prog || !prefilter_prog || !brdf_prog) {
        if (irradiance_prog) glDeleteProgram(irradiance_prog);
        if (prefilter_prog) glDeleteProgram(prefilter_prog);
        if (brdf_prog) glDeleteProgram(brdf_prog);
        delete irradiance;
        delete prefilter;
        delete brdf;
        return false;
    }

    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    GLint prev_viewport[4];
    glGetIntegerv(GL_VIEWPORT, prev_viewport);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    CaptureGeometry geo;
    geo.create();

    // 1) Irradiance（漫反射卷积）
    glUseProgram(irradiance_prog);
    setInt(irradiance_prog, "environmentMap", 0);
    setMat4(irradiance_prog, "projection", kCaptureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, out.environment_cube->id());
    glViewport(0, 0, irradiance_size, irradiance_size);
    for (int face = 0; face < 6; ++face) {
        setMat4(irradiance_prog, "view", kCaptureViews[face]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, irradiance->id(), 0);
        glClear(GL_COLOR_BUFFER_BIT);
        geo.drawCube();
    }

    // 2) Prefilter（按 roughness 分级的镜面预滤波）
    glUseProgram(prefilter_prog);
    setInt(prefilter_prog, "environmentMap", 0);
    setMat4(prefilter_prog, "projection", kCaptureProjection);
    setFloat(prefilter_prog, "resolution", env_resolution);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, out.environment_cube->id());
    for (int mip = 0; mip < prefilter_mips; ++mip) {
        const int mip_size = (int)(prefilter_size * std::pow(0.5f, mip));
        const float roughness = (float)mip / (float)(prefilter_mips - 1);
        glViewport(0, 0, mip_size, mip_size);
        setFloat(prefilter_prog, "roughness", roughness);
        for (int face = 0; face < 6; ++face) {
            setMat4(prefilter_prog, "view", kCaptureViews[face]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, prefilter->id(), mip);
            glClear(GL_COLOR_BUFFER_BIT);
            geo.drawCube();
        }
    }

    // 3) BRDF LUT（全屏一次积分）
    glUseProgram(brdf_prog);
    glViewport(0, 0, brdf_size, brdf_size);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf->id(), 0);
    glClear(GL_COLOR_BUFFER_BIT);
    geo.drawQuad();

    geo.destroy();
    glDeleteProgram(irradiance_prog);
    glDeleteProgram(prefilter_prog);
    glDeleteProgram(brdf_prog);
    glDeleteFramebuffers(1, &fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    out.irradiance_cube = irradiance;
    out.prefilter_cube = prefilter;
    out.brdf_lut = brdf;
    out.ready = true;
    return true;
}
