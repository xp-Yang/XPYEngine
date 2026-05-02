#include "GcodeData.hpp"
#include "GcodeImporter.hpp"

void CachedPosition::reset()
{
    std::fill(position.begin(), position.end(), FLT_MAX);
    feedrate = FLT_MAX;
}

void CpColor::reset()
{
    counter = 0;
    current = 0;
}

void UsedFilaments::reset()
{
    color_change_cache = 0.0f;
    volumes_per_color_change = std::vector<double>();

    tool_change_cache = 0.0f;
    volumes_per_extruder.clear();

    flush_per_filament.clear();

    role_cache = 0.0f;
    filaments_per_role.clear();
}

void UsedFilaments::increase_caches(double extruded_volume)
{
    color_change_cache += extruded_volume;
    tool_change_cache += extruded_volume;
    role_cache += extruded_volume;
}

void UsedFilaments::process_color_change_cache()
{
    if (color_change_cache != 0.0f) {
        volumes_per_color_change.push_back(color_change_cache);
        color_change_cache = 0.0f;
    }
}

void UsedFilaments::process_extruder_cache(GCodeProcessor* processor)
{
    size_t active_extruder_id = processor->m_extruder_id;
    if (tool_change_cache != 0.0f) {
        if (volumes_per_extruder.find(active_extruder_id) != volumes_per_extruder.end())
            volumes_per_extruder[active_extruder_id] += tool_change_cache;
        else
            volumes_per_extruder[active_extruder_id] = tool_change_cache;
        tool_change_cache = 0.0f;
    }
}

void UsedFilaments::update_flush_per_filament(size_t extrude_id, float flush_volume)
{
    if (flush_per_filament.find(extrude_id) != flush_per_filament.end())
        flush_per_filament[extrude_id] += flush_volume;
    else
        flush_per_filament[extrude_id] = flush_volume;
}

void UsedFilaments::process_role_cache(GCodeProcessor* processor)
{
    if (role_cache != 0.0f) {
        std::pair<double, double> filament = { 0.0f, 0.0f };

        double s = Math::Constant::PI * sqr(0.5 * processor->m_result.filament_diameters[processor->m_extruder_id]);
        filament.first = role_cache / s * 0.001;
        filament.second = role_cache * processor->m_result.filament_densities[processor->m_extruder_id] * 0.001;

        ExtrusionRole active_role = processor->m_extrusion_role;
        if (filaments_per_role.find(active_role) != filaments_per_role.end()) {
            filaments_per_role[active_role].first += filament.first;
            filaments_per_role[active_role].second += filament.second;
        }
        else
            filaments_per_role[active_role] = filament;
        role_cache = 0.0f;
    }
}

void UsedFilaments::process_caches(GCodeProcessor* processor)
{
    process_color_change_cache();
    process_extruder_cache(processor);
    process_role_cache(processor);
}
