#include "Render/RenderFrameData.hpp"

void RenderFrameData::reset()
{
    directional_lights.clear();
    point_lights.clear();
    picked_ids.clear();
    point_light_inst_amount = 0;
}
