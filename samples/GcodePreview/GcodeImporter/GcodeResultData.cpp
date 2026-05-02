#include "GcodeResultData.hpp"

void GCodeProcessorResult::reset() {
    //lock();

    moves.clear();
    lines_ends.clear();
    printable_area = Pointfs();
    //BBS: add bed exclude area
    bed_exclude_area = Pointfs();
    //BBS: add toolpath_outside
    toolpath_outside = false;
    //BBS: add label_object_enabled
    label_object_enabled = false;
    printable_height = 0.0f;
    settings_ids.reset();
    extruders_count = 0;
    extruder_colors = std::vector<std::string>();
    filament_diameters = std::vector<float>(MIN_EXTRUDERS_COUNT, DEFAULT_FILAMENT_DIAMETER);
    required_nozzle_HRC = std::vector<int>(MIN_EXTRUDERS_COUNT, DEFAULT_FILAMENT_HRC);
    filament_densities = std::vector<float>(MIN_EXTRUDERS_COUNT, DEFAULT_FILAMENT_DENSITY);
    custom_gcode_per_print_z = std::vector<CustomGCode::Item>();
    spiral_vase_layers = std::vector<std::pair<float, std::pair<size_t, size_t>>>();
    warnings.clear();

    //unlock();
}