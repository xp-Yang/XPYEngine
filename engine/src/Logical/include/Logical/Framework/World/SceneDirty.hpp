#ifndef SceneDirty_hpp
#define SceneDirty_hpp

#include <cstdint>

enum class SceneDirtyFlag : uint32_t {
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

using SceneDirtyFlags = uint32_t;

inline SceneDirtyFlags SceneDirtyFlagBit(SceneDirtyFlag flag)
{
    return static_cast<SceneDirtyFlags>(flag);
}

inline bool HasSceneDirtyFlag(SceneDirtyFlags flags, SceneDirtyFlag flag)
{
    return (flags & SceneDirtyFlagBit(flag)) != 0;
}

#endif // !SceneDirty_hpp
