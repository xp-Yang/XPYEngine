#ifndef RenderDirty_hpp
#define RenderDirty_hpp

#include <cstdint>

enum class RenderDirtyFlag : uint32_t {
    None       = 0,
    Created    = 1 << 0,
    Removed    = 1 << 1,
    Visibility = 1 << 2,
    Transform  = 1 << 3,
    Mesh       = 1 << 4,
    Material   = 1 << 5,
    Light      = 1 << 6,
    Camera     = 1 << 7,
    Picked     = 1 << 8,
    FullResync = 1 << 9,
};

using RenderDirtyFlags = uint32_t;

inline RenderDirtyFlags RenderDirtyFlagBit(RenderDirtyFlag flag)
{
    return static_cast<RenderDirtyFlags>(flag);
}

inline bool HasRenderDirtyFlag(RenderDirtyFlags flags, RenderDirtyFlag flag)
{
    return (flags & RenderDirtyFlagBit(flag)) != 0;
}

#endif // !RenderDirty_hpp
