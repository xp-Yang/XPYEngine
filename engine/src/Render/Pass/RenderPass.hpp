#ifndef RenderPass_hpp
#define RenderPass_hpp

#include "Render/RHI/rhi.hpp"
#include "Render/RenderPipelineLibrary.hpp"

#include <string>
#include <unordered_map>
#include <stdexcept>

class RenderPassContext;

static inline constexpr float DEFAULT_RENDER_RESOLUTION_X = 1920.0f;
static inline constexpr float DEFAULT_RENDER_RESOLUTION_Y = 1080.0f;
// for debug visualization
static inline int PickingColorIDFactor = 256 * 256 * 256 / 500;

// A RenderPass describes draw work; RenderGraph owns the render targets it writes to.
// Passes access RenderGraph resources via named slots injected through bindSlot(),
// rather than hardcoding RGResource/RGTarget names in draw().

class RenderPass {
public:
	enum class Type {
        Unknown,
        
        ZPre,
        Picking,
        Outline,
        SkyBox,
        Shadow,
        Forward,
        GBuffer,
        DeferredLighting,
        SSAO,
        Transparent,
        
        WireFrame,
        CheckerBoard,
        Normal,
        RayTracing,
        
        // post process
        Bloom,
        FXAA,
        ToneMapping,
        ColorGrading,
        
        Final,
	};

	RenderPass()
        : m_rhi(Rhi::get())
        , m_command_buffer(m_rhi ? m_rhi->newCommandBuffer() : nullptr)
    {}
	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;
	virtual ~RenderPass() { delete m_command_buffer; }
	virtual void draw(RenderPassContext& context) = 0;

    void bindSlot(const std::string& slot_name, const std::string& resource_name)
    {
        m_slot_bindings[slot_name] = resource_name;
    }

    const std::string& slot(const std::string& slot_name) const
    {
        auto it = m_slot_bindings.find(slot_name);
        if (it == m_slot_bindings.end())
            throw std::runtime_error("RenderPass slot not bound: " + slot_name);
        return it->second;
    }

    bool hasSlot(const std::string& slot_name) const
    {
        return m_slot_bindings.find(slot_name) != m_slot_bindings.end();
    }

    void clearSlots() { m_slot_bindings.clear(); }

protected:
	Rhi* m_rhi;
    RhiCommandBuffer* m_command_buffer{ nullptr };

	Type m_type;
    std::unordered_map<std::string, std::string> m_slot_bindings;
};

#endif // !RenderPass_hpp
