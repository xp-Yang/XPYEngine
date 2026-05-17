#ifndef RenderGraphDebugInfo_hpp
#define RenderGraphDebugInfo_hpp

#include "Base/Common.hpp"

#include <string>
#include <vector>

struct RenderGraphResourceDebugInfo {
    std::string name;

    std::string owner_pass;
    std::string last_modifier_pass;
    std::string render_target;
    std::string attachment;
    std::string format;

    int color_attachment_index{ 0 };
    int sample_count{ 1 };
    bool transient{ true };
    bool is_depth{ false };

    Vec2 size;
    unsigned int texture_id{ 0 };

    std::vector<std::string> direct_history;
    std::vector<std::string> contributors;
};

#endif // !RenderGraphDebugInfo_hpp
