#include "GcodeImporter.hpp"

#include <float.h>
#include <assert.h>
#include <utility>
#include <chrono>

class CNumericLocalesSetter {
public:
    CNumericLocalesSetter() {
#ifdef _WIN32
        _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
        m_orig_numeric_locale = std::setlocale(LC_NUMERIC, nullptr);
        std::setlocale(LC_NUMERIC, "C");
#elif __APPLE__
        m_original_locale = uselocale((locale_t)0);
        m_new_locale = newlocale(LC_NUMERIC_MASK, "C", m_original_locale);
        uselocale(m_new_locale);
#else // linux / BSD
        m_original_locale = uselocale((locale_t)0);
        m_new_locale = duplocale(m_original_locale);
        m_new_locale = newlocale(LC_NUMERIC_MASK, "C", m_new_locale);
        uselocale(m_new_locale);
#endif
    }
    ~CNumericLocalesSetter() {
#ifdef _WIN32
        std::setlocale(LC_NUMERIC, m_orig_numeric_locale.data());
#else
        uselocale(m_original_locale);
        freelocale(m_new_locale);
#endif
    }

private:
#ifdef _WIN32
    std::string m_orig_numeric_locale;
#else
    locale_t m_original_locale;
    locale_t m_new_locale;
#endif

};

float calc_arc_radian(Vec3f start_pos, Vec3f end_pos, Vec3f center_pos, bool is_ccw)
{
    Vec3f delta1 = center_pos - start_pos;
    Vec3f delta2 = center_pos - end_pos;
    // only consider arc in x-y plane, so clean z distance
    delta1.z = 0;
    delta2.z = 0;

    float radian;
    if (Math::Length(delta1 - delta2) < 1e-6) {
        // start_pos is same with end_pos, we think it's a full circle
        radian = 2 * Math::Constant::PI;
    }
    else {
        double dot = Math::Dot(delta1,delta2);
        double cross = (double)delta1.x * (double)delta2.y - (double)delta1.y * (double)delta2.x;
        radian = atan2(cross, dot);
        if (is_ccw)
            radian = (radian < 0) ? 2 * Math::Constant::PI + radian : radian;
        else
            radian = (radian < 0) ? abs(radian) : 2 * Math::Constant::PI - radian;
    }
    return radian;
}

float calc_arc_radius(Vec3f start_pos, Vec3f center_pos)
{
    Vec3f delta1 = center_pos - start_pos;
    delta1.z = 0;
    return Math::Length(delta1);
}

float calc_arc_length(Vec3f start_pos, Vec3f end_pos, Vec3f center_pos, bool is_ccw)
{
    return calc_arc_radius(start_pos, center_pos) * calc_arc_radian(start_pos, end_pos, center_pos, is_ccw);
}

Vec3f calc_tangential_vector(const Vec3f& pos, const Vec3f& center_pos, const bool is_ccw)
{
    Vec3f dir = center_pos - pos;
    dir.z = 0;
    Math::Normalize(dir);
    Vec3f res;
    if (is_ccw)
        res = { dir.y, -dir.x, 0.0f };
    else
        res = { -dir.y, dir.x, 0.0f };
    return res;
}


bool GCodeProcessor::contains_reserved_tag(const std::string& gcode, std::string& found_tag)
{
    bool ret = false;

    GCodeReader parser;
    parser.parse_buffer(gcode, [&ret, &found_tag](GCodeReader& parser, const GCodeLine& line) {
        std::string comment = line.raw();
        if (comment.length() > 2 && comment.front() == ';') {
            comment = comment.substr(1);
            for (const std::string& s : Reserved_Tags) {
                if (Utils::starts_with(comment, s)) {
                    ret = true;
                    found_tag = comment;
                    parser.quit_parsing();
                    return;
                }
            }
        }
        });

    return ret;
}

bool GCodeProcessor::contains_reserved_tags(const std::string& gcode, unsigned int max_count, std::vector<std::string>& found_tag)
{
    max_count = std::max(max_count, 1U);

    bool ret = false;

    CNumericLocalesSetter locales_setter;

    GCodeReader parser;
    parser.parse_buffer(gcode, [&ret, &found_tag, max_count](GCodeReader& parser, const GCodeLine& line) {
        std::string comment = line.raw();
        if (comment.length() > 2 && comment.front() == ';') {
            comment = comment.substr(1);
            for (const std::string& s : Reserved_Tags) {
                if (Utils::starts_with(comment, s)) {
                    ret = true;
                    found_tag.push_back(comment);
                    if (found_tag.size() == max_count) {
                        parser.quit_parsing();
                        return;
                    }
                }
            }
        }
        });

    return ret;
}

GCodeProcessor::GCodeProcessor()
    : m_options_z_corrector(m_result)
{
    reset();
}

void GCodeProcessor::reset()
{
    m_units = EUnits::Millimeters;
    m_global_positioning_type = EPositioningType::Absolute;
    m_e_local_positioning_type = EPositioningType::Absolute;
    m_extruder_offsets = std::vector<Vec3f>(MIN_EXTRUDERS_COUNT, Vec3f());
    m_flavor = gcfRepRapSprinter;
    m_nozzle_volume = 0.f;

    m_start_position = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_end_position = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_origin = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_cached_position.reset();
    m_wiping = false;
    m_flushing = false;
    m_remaining_volume = 0.f;
    // BBS: arc move related data
    m_move_path_type = EMovePathType::Noop_move;
    m_arc_center = Vec3f();

    m_line_id = 0;
    m_last_line_id = 0;
    m_feedrate = 0.0f;
    m_width = 0.0f;
    m_height = 0.0f;
    m_forced_width = 0.0f;
    m_forced_height = 0.0f;
    m_mm3_per_mm = 0.0f;
    m_fan_speed = 0.0f;

    m_extrusion_role = erNone;
    m_extruder_id = 0;
    m_last_extruder_id = 0;
    m_extruder_colors.resize(MIN_EXTRUDERS_COUNT);
    for (size_t i = 0; i < MIN_EXTRUDERS_COUNT; ++i) {
        m_extruder_colors[i] = static_cast<unsigned char>(i);
    }
    m_extruder_temps.resize(MIN_EXTRUDERS_COUNT);
    for (size_t i = 0; i < MIN_EXTRUDERS_COUNT; ++i) {
        m_extruder_temps[i] = 0.0f;
    }
    m_highest_bed_temp = 0;

    m_extruded_last_z = 0.0f;
    m_zero_layer_height = 0.0f;
    m_first_layer_height = 0.0f;
    m_processing_start_custom_gcode = false;
    m_g1_line_id = 0;
    m_layer_id = 0;
    m_cp_color.reset();

    m_producer = EProducer::Unknown;

    m_used_filaments.reset();

    m_result.reset();
    m_result.id = ++s_result_id;

    m_last_default_color_id = 0;

    m_options_z_corrector.reset();

    m_spiral_vase_active = false;
}

static inline const char* skip_whitespaces(const char* begin, const char* end) {
    for (; begin != end && (*begin == ' ' || *begin == '\t'); ++begin);
    return begin;
}

static inline const char* remove_eols(const char* begin, const char* end) {
    for (; begin != end && (*(end - 1) == '\r' || *(end - 1) == '\n'); --end);
    return end;
}

void GCodeProcessor::apply_config()
{
    // construct a default context

    m_nozzle_volume = 107.f;
    //const ConfigOptionFloat* nozzle_volume = config.option<ConfigOptionFloat>("nozzle_volume");
    //if (nozzle_volume != nullptr)
    //    m_nozzle_volume = nozzle_volume->value;

    m_result.nozzle_hrc = 2;
    //const ConfigOptionInt* nozzle_HRC = config.option<ConfigOptionInt>("nozzle_hrc");
    //if (nozzle_HRC != nullptr) m_result.nozzle_hrc = nozzle_HRC->value;

    m_flavor = GCodeFlavor::gcfMarlinLegacy;
    //const ConfigOptionEnum<GCodeFlavor>* gcode_flavor = config.option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor");
    //if (gcode_flavor != nullptr)
    //    m_flavor = gcode_flavor->value;

    m_result.printable_area = { Vec2f(0,0), Vec2f(256,0), Vec2f(256,256), Vec2f(0,256) };
    //const ConfigOptionPoints* printable_area = config.option<ConfigOptionPoints>("printable_area");
    //if (printable_area != nullptr)
    //    m_result.printable_area = printable_area->values;

    m_result.bed_exclude_area = { Vec2f(0,0), Vec2f(18,0), Vec2f(18,28), Vec2f(0,28) };
    //const ConfigOptionPoints* bed_exclude_area = config.option<ConfigOptionPoints>("bed_exclude_area");
    //if (bed_exclude_area != nullptr)
    //    m_result.bed_exclude_area = bed_exclude_area->values;

    m_result.settings_ids.print = "0.20mm Standard @BBL X1C";
    //const ConfigOptionString* print_settings_id = config.option<ConfigOptionString>("print_settings_id");
    //if (print_settings_id != nullptr)
    //    m_result.settings_ids.print = print_settings_id->value;

    m_result.settings_ids.filament = { "Bambu PLA Basic @BBL X1C" };
    //const ConfigOptionStrings* filament_settings_id = config.option<ConfigOptionStrings>("filament_settings_id");
    //if (filament_settings_id != nullptr)
    //    m_result.settings_ids.filament = filament_settings_id->values;

    m_result.settings_ids.printer = "Bambu Lab X1 Carbon 0.4 nozzle";
    //const ConfigOptionString* printer_settings_id = config.option<ConfigOptionString>("printer_settings_id");
    //if (printer_settings_id != nullptr)
    //    m_result.settings_ids.printer = printer_settings_id->value;

    m_result.extruders_count = 1;

    m_result.filament_diameters = { 1.75000000 };
    //const ConfigOptionFloats* filament_diameters = config.option<ConfigOptionFloats>("filament_diameter");
    //if (filament_diameters != nullptr) {
    //    m_result.filament_diameters.clear();
    //    m_result.filament_diameters.resize(filament_diameters->values.size());
    //    for (size_t i = 0; i < filament_diameters->values.size(); ++i) {
    //        m_result.filament_diameters[i] = static_cast<float>(filament_diameters->values[i]);
    //    }
    //}
    //if (m_result.filament_diameters.size() < m_result.extruders_count) {
    //    for (size_t i = m_result.filament_diameters.size(); i < m_result.extruders_count; ++i) {
    //        m_result.filament_diameters.emplace_back(DEFAULT_FILAMENT_DIAMETER);
    //    }
    //}

    m_result.required_nozzle_HRC = { 3 };
    //const ConfigOptionInts* filament_HRC = config.option<ConfigOptionInts>("required_nozzle_HRC");
    //if (filament_HRC != nullptr) {
    //    m_result.required_nozzle_HRC.clear();
    //    m_result.required_nozzle_HRC.resize(filament_HRC->values.size());
    //    for (size_t i = 0; i < filament_HRC->values.size(); ++i) { m_result.required_nozzle_HRC[i] = static_cast<float>(filament_HRC->values[i]); }
    //}
    //if (m_result.required_nozzle_HRC.size() < m_result.extruders_count) {
    //    for (size_t i = m_result.required_nozzle_HRC.size(); i < m_result.extruders_count; ++i) {
    //        m_result.required_nozzle_HRC.emplace_back(DEFAULT_FILAMENT_HRC);
    //    }
    //}

    m_result.filament_densities = { 1.25999999 };
    //const ConfigOptionFloats* filament_densities = config.option<ConfigOptionFloats>("filament_density");
    //if (filament_densities != nullptr) {
    //    m_result.filament_densities.clear();
    //    m_result.filament_densities.resize(filament_densities->values.size());
    //    for (size_t i = 0; i < filament_densities->values.size(); ++i) {
    //        m_result.filament_densities[i] = static_cast<float>(filament_densities->values[i]);
    //    }
    //}
    //if (m_result.filament_densities.size() < m_result.extruders_count) {
    //    for (size_t i = m_result.filament_densities.size(); i < m_result.extruders_count; ++i) {
    //        m_result.filament_densities.emplace_back(DEFAULT_FILAMENT_DENSITY);
    //    }
    //}
    
    m_result.filament_vitrification_temperature = { 45 };
    //const ConfigOptionInts* filament_vitrification_temperature = config.option<ConfigOptionInts>("temperature_vitrification");
    //if (filament_vitrification_temperature != nullptr) {
    //    m_result.filament_vitrification_temperature.clear();
    //    m_result.filament_vitrification_temperature.resize(filament_vitrification_temperature->values.size());
    //    for (size_t i = 0; i < filament_vitrification_temperature->values.size(); ++i) {
    //        m_result.filament_vitrification_temperature[i] = static_cast<int>(filament_vitrification_temperature->values[i]);
    //    }
    //}
    //if (m_result.filament_vitrification_temperature.size() < m_result.extruders_count) {
    //    for (size_t i = m_result.filament_vitrification_temperature.size(); i < m_result.extruders_count; ++i) {
    //        m_result.filament_vitrification_temperature.emplace_back(DEFAULT_FILAMENT_VITRIFICATION_TEMPERATURE);
    //    }
    //}

    m_extruder_offsets = { Vec3f(0,2,0) };
    //const ConfigOptionPoints* extruder_offset = config.option<ConfigOptionPoints>("extruder_offset");
    //const ConfigOptionBool* single_extruder_multi_material = config.option<ConfigOptionBool>("single_extruder_multi_material");
    //if (extruder_offset != nullptr) {
    //    //BBS: for single extruder multi material, only use the offset of first extruder
    //    if (single_extruder_multi_material != nullptr && single_extruder_multi_material->getBool()) {
    //        Vec2f offset = extruder_offset->values[0].cast<float>();
    //        m_extruder_offsets.resize(m_result.extruders_count);
    //        for (size_t i = 0; i < m_result.extruders_count; ++i) {
    //            m_extruder_offsets[i] = { offset.x, offset.y, 0.0f };
    //        }
    //    }
    //    else {
    //        m_extruder_offsets.resize(extruder_offset->values.size());
    //        for (size_t i = 0; i < extruder_offset->values.size(); ++i) {
    //            Vec2f offset = extruder_offset->values[i].cast<float>();
    //            m_extruder_offsets[i] = { offset.x, offset.y, 0.0f };
    //        }
    //    }
    //}
    //if (m_extruder_offsets.size() < m_result.extruders_count) {
    //    for (size_t i = m_extruder_offsets.size(); i < m_result.extruders_count; ++i) {
    //        m_extruder_offsets.emplace_back(DEFAULT_EXTRUDER_OFFSET);
    //    }
    //}

    m_result.extruder_colors = { "" };
    //const ConfigOptionStrings* filament_colour = config.option<ConfigOptionStrings>("filament_colour");
    //if (filament_colour != nullptr && filament_colour->values.size() == m_result.extruder_colors.size()) {
    //    for (size_t i = 0; i < m_result.extruder_colors.size(); ++i) {
    //        if (m_result.extruder_colors[i].empty())
    //            m_result.extruder_colors[i] = filament_colour->values[i];
    //    }
    //}
    //if (m_result.extruder_colors.size() < m_result.extruders_count) {
    //    for (size_t i = m_result.extruder_colors.size(); i < m_result.extruders_count; ++i) {
    //        m_result.extruder_colors.emplace_back(std::string());
    //    }
    //}

    // replace missing values with default
    for (size_t i = 0; i < m_result.extruder_colors.size(); ++i) {
        if (m_result.extruder_colors[i].empty())
            m_result.extruder_colors[i] = "#FF8000";
    }

    m_extruder_colors.resize(m_result.extruder_colors.size());
    for (size_t i = 0; i < m_result.extruder_colors.size(); ++i) {
        m_extruder_colors[i] = static_cast<unsigned char>(i);
    }

    m_extruder_temps.resize(m_result.extruders_count);

    m_first_layer_height = 0.200000003;
    //const ConfigOptionFloat* initial_layer_print_height = config.option<ConfigOptionFloat>("initial_layer_print_height");
    //if (initial_layer_print_height != nullptr)
    //    m_first_layer_height = std::abs(initial_layer_print_height->value);

    m_result.printable_height = 250.0;
    //const ConfigOptionFloat* printable_height = config.option<ConfigOptionFloat>("printable_height");
    //if (printable_height != nullptr)
    //    m_result.printable_height = printable_height->value;

    m_spiral_vase_active = false;
    //const ConfigOptionBool* spiral_vase = config.option<ConfigOptionBool>("spiral_mode");
    //if (spiral_vase != nullptr)
    //    m_spiral_vase_active = spiral_vase->value;

    m_result.bed_type = BedType::btPC;
    //const ConfigOptionEnumGeneric* bed_type = config.option<ConfigOptionEnumGeneric>("curr_bed_type");
    //if (bed_type != nullptr)
    //    m_result.bed_type = (BedType)bed_type->value;
}

// Load a G-code into a stand-alone G-code viewer.
// throws CanceledException through print->throw_if_canceled() (sent by the caller as callback).
void GCodeProcessor::process_file(const std::string& filename, std::function<void()> cancel_callback)
{
    CNumericLocalesSetter locales_setter;

    // pre-processing
    // parse the gcode file to detect its producer
    {
        m_parser.parse_file_raw(filename, [this](GCodeReader& reader, const char* begin, const char* end) {
            begin = skip_whitespaces(begin, end);
            if (begin != end && *begin == ';') {
                // Comment.
                begin = skip_whitespaces(++begin, end);
                end = remove_eols(begin, end);
                if (begin != end) {
                    if (m_producer == EProducer::Unknown) {
                        if (detect_producer(std::string_view(begin, end - begin))) {
                            m_parser.quit_parsing();
                        }
                    }
                    else if (std::string(begin, end).find("CONFIG_BLOCK_END") != std::string::npos) {
                        m_parser.quit_parsing();
                    }
                }
            }
            });
        m_parser.reset();

        // if the gcode was produced by BambuStudio,
        // extract the config from it
        if (m_producer == EProducer::BambuStudio || m_producer == EProducer::Slic3rPE || m_producer == EProducer::Slic3r) {
            apply_config();
        }
        //else if (m_producer == EProducer::Simplify3D)
        //    apply_config_simplify3d(filename);
        //else if (m_producer == EProducer::SuperSlicer)
        //    apply_config_superslicer(filename);
    }

    // process gcode
    m_result.filename = filename;
    m_result.id = ++s_result_id;
    // 1st move must be a dummy move
    m_result.moves.emplace_back(MoveVertex());
    size_t parse_line_callback_cntr = 10000;
    m_parser.parse_file(filename, [this, cancel_callback, &parse_line_callback_cntr](GCodeReader& reader, const GCodeLine& line) {
        if (--parse_line_callback_cntr == 0) {
            // Don't call the cancel_callback() too often, do it every at every 10000'th line.
            parse_line_callback_cntr = 10000;
            if (cancel_callback)
                cancel_callback();
        }
        this->process_gcode_line(line, true);
        }, m_result.lines_ends);

    // Don't post-process the G-code to update time stamps.
    this->finalize(false);
}

void GCodeProcessor::initialize(const std::string& filename)
{
    assert(is_decimal_separator_point());

    // process gcode
    m_result.filename = filename;
    m_result.id = ++s_result_id;
    // 1st move must be a dummy move
    m_result.moves.emplace_back(MoveVertex());
}

void GCodeProcessor::process_buffer(const std::string& buffer)
{
    //FIXME maybe cache GCodeLine gline to be over multiple parse_buffer() invocations.
    m_parser.parse_buffer(buffer, [this](GCodeReader&, const GCodeLine& line) {
        this->process_gcode_line(line, false);
        });
}

void GCodeProcessor::finalize(bool post_process)
{
    // update width/height of wipe moves
    for (MoveVertex& move : m_result.moves) {
        if (move.type == EMoveType::Wipe) {
            move.width = Wipe_Width;
            move.height = Wipe_Height;
        }
    }

    m_used_filaments.process_caches(this);

    update_estimated_times_stats();
    auto time_mode = m_result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

    auto it = std::find_if(time_mode.roles_times.begin(), time_mode.roles_times.end(), [](const std::pair<ExtrusionRole, float>& item) { return erCustom == item.first; });
    auto prepare_time = (it != time_mode.roles_times.end()) ? it->second : 0.0f;

    //update times for results
    for (size_t i = 0; i < m_result.moves.size(); i++) {
        //field layer_duration contains the layer id for the move in which the layer_duration has to be set.
        size_t layer_id = size_t(m_result.moves[i].layer_duration);
        std::vector<float>& layer_times = m_result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].layers_times;
        if (layer_times.size() > layer_id - 1 && layer_id > 0)
            m_result.moves[i].layer_duration = layer_id == 1 ? std::max(0.f, layer_times[layer_id - 1] - prepare_time) : layer_times[layer_id - 1];
        else
            m_result.moves[i].layer_duration = 0;
    }

    //BBS: update slice warning
    update_slice_warnings();
}

void GCodeProcessor::process_gcode_line(const GCodeLine& line, bool producers_enabled)
{
    /* std::cout << line.raw() << std::endl; */

    ++m_line_id;

    // update start position
    m_start_position = m_end_position;

    const std::string_view cmd = line.cmd();
    //OrcaSlicer
    if (m_flavor == gcfKlipper)
    {
        if (Utils::iequals(std::string(cmd), "SET_VELOCITY_LIMIT"))
        {
            process_SET_VELOCITY_LIMIT(line);
            return;
        }
    }

    if (cmd.length() > 1) {
        // process command lines
        switch (cmd[0])
        {
        case 'g':
        case 'G':
            switch (cmd.size()) {
            case 2:
                switch (cmd[1]) {
                case '0': { process_G0(line); break; }  // Move
                case '1': { process_G1(line); break; }  // Move
                case '2':
                case '3': { process_G2_G3(line); break; }  // Move
                        //BBS
                case '4': { process_G4(line); break; }  // Delay
                default: break;
                }
                break;
            case 3:
                switch (cmd[1]) {
                case '1':
                    switch (cmd[2]) {
                    case '0': { process_G10(line); break; } // Retract
                    case '1': { process_G11(line); break; } // Unretract
                    default: break;
                    }
                    break;
                case '2':
                    switch (cmd[2]) {
                    case '0': { process_G20(line); break; } // Set Units to Inches
                    case '1': { process_G21(line); break; } // Set Units to Millimeters
                    case '2': { process_G22(line); break; } // Firmware controlled retract
                    case '3': { process_G23(line); break; } // Firmware controlled unretract
                    case '8': { process_G28(line); break; } // Move to origin
                    case '9': { process_G29(line); break; }
                    default: break;
                    }
                    break;
                case '9':
                    switch (cmd[2]) {
                    case '0': { process_G90(line); break; } // Set to Absolute Positioning
                    case '1': { process_G91(line); break; } // Set to Relative Positioning
                    case '2': { process_G92(line); break; } // Set Position
                    default: break;
                    }
                    break;
                }
                break;
            default:
                break;
            }
            break;
        case 'm':
        case 'M':
            switch (cmd.size()) {
            case 2:
                switch (cmd[1]) {
                case '1': { process_M1(line); break; }   // Sleep or Conditional stop
                default: break;
                }
                break;
            case 3:
                switch (cmd[1]) {
                case '8':
                    switch (cmd[2]) {
                    case '2': { process_M82(line); break; }  // Set extruder to absolute mode
                    case '3': { process_M83(line); break; }  // Set extruder to relative mode
                    default: break;
                    }
                    break;
                default:
                    break;
                }
                break;
            case 4:
                switch (cmd[1]) {
                case '1':
                    switch (cmd[2]) {
                    case '0':
                        switch (cmd[3]) {
                        case '4': { process_M104(line); break; } // Set extruder temperature
                        case '6': { process_M106(line); break; } // Set fan speed
                        case '7': { process_M107(line); break; } // Disable fan
                        case '8': { process_M108(line); break; } // Set tool (Sailfish)
                        case '9': { process_M109(line); break; } // Set extruder temperature and wait
                        default: break;
                        }
                        break;
                    case '3':
                        switch (cmd[3]) {
                        case '2': { process_M132(line); break; } // Recall stored home offsets
                        case '5': { process_M135(line); break; } // Set tool (MakerWare)
                        default: break;
                        }
                        break;
                    case '4':
                        switch (cmd[3]) {
                        case '0': { process_M140(line); break; } // Set bed temperature
                        default: break;
                        }
                    case '9':
                        switch (cmd[3]) {
                        case '0': { process_M190(line); break; } // Wait bed temperature
                        default: break;
                        }
                    default:
                        break;
                    }
                    break;
                case '2':
                    switch (cmd[2]) {
                    case '0':
                        switch (cmd[3]) {
                        case '1': { process_M201(line); break; } // Set max printing acceleration
                        case '3': { process_M203(line); break; } // Set maximum feedrate
                        case '4': { process_M204(line); break; } // Set default acceleration
                        case '5': { process_M205(line); break; } // Advanced settings
                        default: break;
                        }
                        break;
                    case '2':
                        switch (cmd[3]) {
                        case '1': { process_M221(line); break; } // Set extrude factor override percentage
                        default: break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case '4':
                    switch (cmd[2]) {
                    case '0':
                        switch (cmd[3]) {
                            //BBS
                        case '0': { process_M400(line); break; } // BBS delay
                        case '1': { process_M401(line); break; } // Repetier: Store x, y and z position
                        case '2': { process_M402(line); break; } // Repetier: Go to stored position
                        default: break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case '5':
                    switch (cmd[2]) {
                    case '6':
                        switch (cmd[3]) {
                        case '6': { process_M566(line); break; } // Set allowable instantaneous speed change
                        default: break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case '7':
                    switch (cmd[2]) {
                    case '0':
                        switch (cmd[3]) {
                        case '2': { process_M702(line); break; } // Unload the current filament into the MK3 MMU2 unit at the end of print.
                        default: break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
            break;
        case 't':
        case 'T':
            process_T(line); // Select Tool
            break;
        default:
            break;
        }
    }
    else {
        const std::string& comment = line.raw();
        if (comment.length() > 2 && comment.front() == ';')
            // Process tags embedded into comments. Tag comments always start at the start of a line
            // with a comment and continue with a tag without any whitespace separator.
            process_tags(comment.substr(1), producers_enabled);
    }
}

#if __has_include(<charconv>)
template <typename T, typename = void>
struct is_from_chars_convertible : std::false_type {};
template <typename T>
struct is_from_chars_convertible<T, std::void_t<decltype(std::from_chars(std::declval<const char*>(), std::declval<const char*>(), std::declval<T&>()))>> : std::true_type {};
#endif

// Returns true if the number was parsed correctly into out and the number spanned the whole input string.
template<typename T>
[[nodiscard]] static inline bool parse_number(const std::string_view sv, T& out)
{
    // https://www.bfilipek.com/2019/07/detect-overload-from-chars.html#example-stdfromchars
#if __has_include(<charconv>)
// Visual Studio 19 supports from_chars all right.
// OSX compiler that we use only implements std::from_chars just for ints.
// GCC that we compile on does not provide <charconv> at all.
    if constexpr (is_from_chars_convertible<T>::value) {
        auto str_end = sv.data() + sv.size();
        auto [end_ptr, error_code] = std::from_chars(sv.data(), str_end, out);
        return error_code == std::errc() && end_ptr == str_end;
    }
    else
#endif
    {
        // Legacy conversion, which is costly due to having to make a copy of the string before conversion.
        try {
            assert(sv.size() < 1024);
            assert(sv.data() != nullptr);
            std::string str{ sv };
            size_t read = 0;
            if constexpr (std::is_same_v<T, int>)
                out = std::stoi(str, &read);
            else if constexpr (std::is_same_v<T, long>)
                out = std::stol(str, &read);
            else if constexpr (std::is_same_v<T, float>)
                out = string_to_double_decimal_point(str, &read);
            else if constexpr (std::is_same_v<T, double>)
                out = string_to_double_decimal_point(str, &read);
            return str.size() == read;
        }
        catch (...) {
            return false;
        }
    }
}

int GCodeProcessor::get_gcode_last_filament(const std::string& gcode_str)
{
    int str_size = gcode_str.size();
    int start_index = 0;
    int end_index = 0;
    int out_filament = -1;
    while (end_index < str_size) {
        if (gcode_str[end_index] != '\n') {
            end_index++;
            continue;
        }

        if (end_index > start_index) {
            std::string line_str = gcode_str.substr(start_index, end_index - start_index);
            line_str.erase(0, line_str.find_first_not_of(" "));
            line_str.erase(line_str.find_last_not_of(" ") + 1);
            if (line_str.empty() || line_str[0] != 'T') {
                start_index = end_index + 1;
                end_index = start_index;
                continue;
            }

            int out = -1;
            if (parse_number(line_str.substr(1), out) && out >= 0 && out < 255)
                out_filament = out;
        }

        start_index = end_index + 1;
        end_index = start_index;
    }

    return out_filament;
}

//BBS: get last z position from gcode
bool GCodeProcessor::get_last_z_from_gcode(const std::string& gcode_str, double& z)
{
    int str_size = gcode_str.size();
    int start_index = 0;
    int end_index = 0;
    bool is_z_changed = false;
    while (end_index < str_size) {
        //find a full line
        if (gcode_str[end_index] != '\n') {
            end_index++;
            continue;
        }
        //parse the line
        if (end_index > start_index) {
            std::string line_str = gcode_str.substr(start_index, end_index - start_index);
            line_str.erase(0, line_str.find_first_not_of(" "));
            line_str.erase(line_str.find_last_not_of(";") + 1);
            line_str.erase(line_str.find_last_not_of(" ") + 1);

            //command which may have z movement
            if (line_str.size() > 5 && (line_str.find("G0 ") == 0
                || line_str.find("G1 ") == 0
                || line_str.find("G2 ") == 0
                || line_str.find("G3 ") == 0))
            {
                auto z_pos = line_str.find(" Z");
                double temp_z = 0;
                if (z_pos != line_str.npos
                    && z_pos + 2 < line_str.size()) {
                    // Try to parse the numeric value.
                    std::string z_sub = line_str.substr(z_pos + 2);
                    char* c = &z_sub[0];
                    char* end = c + sizeof(z_sub.c_str());

                    auto is_end_of_word = [](char c) {
                        return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0 || c == ';';
                    };

                    auto [pend, ec] = fast_float::from_chars(c, end, temp_z);
                    if (pend != c && is_end_of_word(*pend)) {
                        // The axis value has been parsed correctly.
                        z = temp_z;
                        is_z_changed = true;
                    }
                }
            }
        }
        //loop to handle next line
        start_index = end_index + 1;
        end_index = start_index;
    }
    return is_z_changed;
}

void GCodeProcessor::process_tags(const std::string_view comment, bool producers_enabled)
{
    // producers tags
    if (producers_enabled && process_producers_tags(comment))
        return;

    // extrusion role tag
    if (Utils::starts_with(comment, reserved_tag(ETags::Role))) {
        set_extrusion_role(string_to_extrusion_role(comment.substr(reserved_tag(ETags::Role).length())));
        if (m_extrusion_role == erExternalPerimeter)
            m_seams_detector.activate(true);
        m_processing_start_custom_gcode = (m_extrusion_role == erCustom && m_g1_line_id == 0);
        return;
    }

    // wipe start tag
    if (Utils::starts_with(comment, reserved_tag(ETags::Wipe_Start))) {
        m_wiping = true;
        return;
    }

    // wipe end tag
    if (Utils::starts_with(comment, reserved_tag(ETags::Wipe_End))) {
        m_wiping = false;
        return;
    }

    //BBS: flush start tag
    if (Utils::starts_with(comment, GCodeProcessor::Flush_Start_Tag)) {
        m_flushing = true;
        return;
    }

    //BBS: flush end tag
    if (Utils::starts_with(comment, GCodeProcessor::Flush_End_Tag)) {
        m_flushing = false;
        return;
    }

    if (!producers_enabled || m_producer == EProducer::BambuStudio) {
        // height tag
        if (Utils::starts_with(comment, reserved_tag(ETags::Height))) {
            if (!parse_number(comment.substr(reserved_tag(ETags::Height).size()), m_forced_height))
                assert(false); //BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Height (" << comment << ").";
            return;
        }
        // width tag
        if (Utils::starts_with(comment, reserved_tag(ETags::Width))) {
            if (!parse_number(comment.substr(reserved_tag(ETags::Width).size()), m_forced_width))
                assert(false); //BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Width (" << comment << ").";
            return;
        }
    }

    // color change tag
    if (Utils::starts_with(comment, reserved_tag(ETags::Color_Change))) {
        unsigned char extruder_id = 0;
        static std::vector<std::string> Default_Colors = {
            "#0B2C7A", // { 0.043f, 0.173f, 0.478f }, // bluish
            "#1C8891", // { 0.110f, 0.533f, 0.569f },
            "#AAF200", // { 0.667f, 0.949f, 0.000f },
            "#F5CE0A", // { 0.961f, 0.808f, 0.039f },
            "#D16830", // { 0.820f, 0.408f, 0.188f },
            "#942616", // { 0.581f, 0.149f, 0.087f }  // reddish
        };

        std::string color = Default_Colors[0];
        auto is_valid_color = [](const std::string& color) {
            auto is_hex_digit = [](char c) {
                return ((c >= '0' && c <= '9') ||
                    (c >= 'A' && c <= 'F') ||
                    (c >= 'a' && c <= 'f'));
            };

            if (color[0] != '#' || color.length() != 7)
                return false;
            for (int i = 1; i <= 6; ++i) {
                if (!is_hex_digit(color[i]))
                    return false;
            }
            return true;
        };

        std::vector<std::string> tokens;
        Utils::split(tokens, std::string(comment), ',', true);
        if (tokens.size() > 1) {
            if (tokens[1][0] == 'T') {
                int eid;
                if (!parse_number(tokens[1].substr(1), eid) || eid < 0 || eid > 255) {
                    assert(false); //BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Color_Change (" << comment << ").";
                    return;
                }
                extruder_id = static_cast<unsigned char>(eid);
            }
        }
        if (tokens.size() > 2) {
            if (is_valid_color(tokens[2]))
                color = tokens[2];
        }
        else {
            color = Default_Colors[m_last_default_color_id];
            ++m_last_default_color_id;
            if (m_last_default_color_id == Default_Colors.size())
                m_last_default_color_id = 0;
        }

        if (extruder_id < m_extruder_colors.size())
            m_extruder_colors[extruder_id] = static_cast<unsigned char>(m_extruder_offsets.size()) + m_cp_color.counter; // color_change position in list of color for preview
        ++m_cp_color.counter;
        if (m_cp_color.counter == UCHAR_MAX)
            m_cp_color.counter = 0;

        if (m_extruder_id == extruder_id) {
            m_cp_color.current = m_extruder_colors[extruder_id];
            store_move_vertex(EMoveType::Color_change);
            CustomGCode::Item item = { static_cast<double>(m_end_position[2]), CustomGCode::ColorChange, extruder_id + 1, color, "" };
            m_result.custom_gcode_per_print_z.emplace_back(item);
            m_options_z_corrector.set();
            process_custom_gcode_time(CustomGCode::ColorChange);
            process_filaments(CustomGCode::ColorChange);
        }

        return;
    }

    // pause print tag
    if (comment == reserved_tag(ETags::Pause_Print)) {
        store_move_vertex(EMoveType::Pause_Print);
        CustomGCode::Item item = { static_cast<double>(m_end_position[2]), CustomGCode::PausePrint, m_extruder_id + 1, "", "" };
        m_result.custom_gcode_per_print_z.emplace_back(item);
        m_options_z_corrector.set();
        process_custom_gcode_time(CustomGCode::PausePrint);
        return;
    }

    // custom code tag
    if (comment == reserved_tag(ETags::Custom_Code)) {
        store_move_vertex(EMoveType::Custom_GCode);
        CustomGCode::Item item = { static_cast<double>(m_end_position[2]), CustomGCode::Custom, m_extruder_id + 1, "", "" };
        m_result.custom_gcode_per_print_z.emplace_back(item);
        m_options_z_corrector.set();
        return;
    }

    // layer change tag
    if (comment == reserved_tag(ETags::Layer_Change)) {
        ++m_layer_id;
        if (m_spiral_vase_active) {
            if (m_result.moves.empty() || m_result.spiral_vase_layers.empty())
                // add a placeholder for layer height. the actual value will be set inside process_G1() method
                m_result.spiral_vase_layers.push_back({ FLT_MAX, { 0, 0 } });
            else {
                const size_t move_id = m_result.moves.size() - 1;
                if (!m_result.spiral_vase_layers.empty())
                    m_result.spiral_vase_layers.back().second.second = move_id;
                // add a placeholder for layer height. the actual value will be set inside process_G1() method
                m_result.spiral_vase_layers.push_back({ FLT_MAX, { move_id, move_id } });
            }
        }
        return;
    }
}

bool GCodeProcessor::process_producers_tags(const std::string_view comment)
{
    switch (m_producer)
    {
    case EProducer::Slic3rPE:
    case EProducer::Slic3r:
    case EProducer::SuperSlicer:
    case EProducer::BambuStudio: { return process_bambuslicer_tags(comment); }
    case EProducer::Cura: { return process_cura_tags(comment); }
    case EProducer::Simplify3D: { return process_simplify3d_tags(comment); }
    case EProducer::CraftWare: { return process_craftware_tags(comment); }
    case EProducer::ideaMaker: { return process_ideamaker_tags(comment); }
    case EProducer::KissSlicer: { return process_kissslicer_tags(comment); }
    default: { return false; }
    }
}

bool GCodeProcessor::process_bambuslicer_tags(const std::string_view comment)
{
    return false;
}

bool GCodeProcessor::process_cura_tags(const std::string_view comment)
{
    // TYPE -> extrusion role
    std::string tag = "TYPE:";
    size_t pos = comment.find(tag);
    if (pos != comment.npos) {
        const std::string_view type = comment.substr(pos + tag.length());
        if (type == "SKIRT")
            set_extrusion_role(erSkirt);
        else if (type == "WALL-OUTER")
            set_extrusion_role(erExternalPerimeter);
        else if (type == "WALL-INNER")
            set_extrusion_role(erPerimeter);
        else if (type == "SKIN")
            set_extrusion_role(erSolidInfill);
        else if (type == "FILL")
            set_extrusion_role(erInternalInfill);
        else if (type == "SUPPORT")
            set_extrusion_role(erSupportMaterial);
        else if (type == "SUPPORT-INTERFACE")
            set_extrusion_role(erSupportMaterialInterface);
        else if (type == "PRIME-TOWER")
            set_extrusion_role(erWipeTower);
        else {
            set_extrusion_role(erNone);
            assert(false); //BOOST_LOG_TRIVIAL(warning) << "GCodeProcessor found unknown extrusion role: " << type;
        }

        if (m_extrusion_role == erExternalPerimeter)
            m_seams_detector.activate(true);

        return true;
    }

    // flavor
    tag = "FLAVOR:";
    pos = comment.find(tag);
    if (pos != comment.npos) {
        const std::string_view flavor = comment.substr(pos + tag.length());
        if (flavor == "BFB")
            m_flavor = gcfMarlinLegacy; // is this correct ?
        else if (flavor == "Mach3")
            m_flavor = gcfMach3;
        else if (flavor == "Makerbot")
            m_flavor = gcfMakerWare;
        else if (flavor == "UltiGCode")
            m_flavor = gcfMarlinLegacy; // is this correct ?
        else if (flavor == "Marlin(Volumetric)")
            m_flavor = gcfMarlinLegacy; // is this correct ?
        else if (flavor == "Griffin")
            m_flavor = gcfMarlinLegacy; // is this correct ?
        else if (flavor == "Repetier")
            m_flavor = gcfRepetier;
        else if (flavor == "RepRap")
            m_flavor = gcfRepRapFirmware;
        else if (flavor == "Marlin")
            m_flavor = gcfMarlinLegacy;

        return true;
    }

    // layer
    tag = "LAYER:";
    pos = comment.find(tag);
    if (pos != comment.npos) {
        ++m_layer_id;
        return true;
    }

    return false;
}

bool GCodeProcessor::process_simplify3d_tags(const std::string_view comment)
{
    // extrusion roles

    // in older versions the comments did not contain the key 'feature'
    std::string_view cmt = comment;
    size_t pos = cmt.find(" feature");
    if (pos == 0)
        cmt.remove_prefix(8);

    // ; skirt
    pos = cmt.find(" skirt");
    if (pos == 0) {
        set_extrusion_role(erSkirt);
        return true;
    }

    // ; outer perimeter
    pos = cmt.find(" outer perimeter");
    if (pos == 0) {
        set_extrusion_role(erExternalPerimeter);
        m_seams_detector.activate(true);
        return true;
    }

    // ; inner perimeter
    pos = cmt.find(" inner perimeter");
    if (pos == 0) {
        set_extrusion_role(erPerimeter);
        return true;
    }

    // ; gap fill
    pos = cmt.find(" gap fill");
    if (pos == 0) {
        set_extrusion_role(erGapFill);
        return true;
    }

    // ; infill
    pos = cmt.find(" infill");
    if (pos == 0) {
        set_extrusion_role(erInternalInfill);
        return true;
    }

    // ; solid layer
    pos = cmt.find(" solid layer");
    if (pos == 0) {
        set_extrusion_role(erSolidInfill);
        return true;
    }

    // ; bridge
    pos = cmt.find(" bridge");
    if (pos == 0) {
        set_extrusion_role(erBridgeInfill);
        return true;
    }

    // ; support
    pos = cmt.find(" support");
    if (pos == 0) {
        set_extrusion_role(erSupportMaterial);
        return true;
    }

    // ; dense support
    pos = cmt.find(" dense support");
    if (pos == 0) {
        set_extrusion_role(erSupportMaterialInterface);
        return true;
    }

    // ; prime pillar
    pos = cmt.find(" prime pillar");
    if (pos == 0) {
        set_extrusion_role(erWipeTower);
        return true;
    }

    // ; ooze shield
    pos = cmt.find(" ooze shield");
    if (pos == 0) {
        set_extrusion_role(erNone); // Missing mapping
        return true;
    }

    // ; raft
    pos = cmt.find(" raft");
    if (pos == 0) {
        set_extrusion_role(erSupportMaterial);
        return true;
    }

    // ; internal single extrusion
    pos = cmt.find(" internal single extrusion");
    if (pos == 0) {
        set_extrusion_role(erNone); // Missing mapping
        return true;
    }

    // geometry
    // ; tool
    std::string tag = " tool";
    pos = cmt.find(tag);
    if (pos == 0) {
        const std::string_view data = cmt.substr(pos + tag.length());
        std::string h_tag = "H";
        size_t h_start = data.find(h_tag);
        size_t h_end = data.find_first_of(' ', h_start);
        std::string w_tag = "W";
        size_t w_start = data.find(w_tag);
        size_t w_end = data.find_first_of(' ', w_start);
        if (h_start != data.npos) {
            if (!parse_number(data.substr(h_start + 1, (h_end != data.npos) ? h_end - h_start - 1 : h_end), m_forced_height))
                assert(false);// BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Height (" << comment << ").";
        }
        if (w_start != data.npos) {
            if (!parse_number(data.substr(w_start + 1, (w_end != data.npos) ? w_end - w_start - 1 : w_end), m_forced_width))
                assert(false);// BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Width (" << comment << ").";
        }

        return true;
    }

    // ; layer
    tag = " layer";
    pos = cmt.find(tag);
    if (pos == 0) {
        // skip lines "; layer end"
        const std::string_view data = cmt.substr(pos + tag.length());
        size_t end_start = data.find("end");
        if (end_start == data.npos)
            ++m_layer_id;

        return true;
    }

    return false;
}

bool GCodeProcessor::process_craftware_tags(const std::string_view comment)
{
    // segType -> extrusion role
    std::string tag = "segType:";
    size_t pos = comment.find(tag);
    if (pos != comment.npos) {
        const std::string_view type = comment.substr(pos + tag.length());
        if (type == "Skirt")
            set_extrusion_role(erSkirt);
        else if (type == "Perimeter")
            set_extrusion_role(erExternalPerimeter);
        else if (type == "HShell")
            set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        else if (type == "InnerHair")
            set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        else if (type == "Loop")
            set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        else if (type == "Infill")
            set_extrusion_role(erInternalInfill);
        else if (type == "Raft")
            set_extrusion_role(erSkirt);
        else if (type == "Support")
            set_extrusion_role(erSupportMaterial);
        else if (type == "SupportTouch")
            set_extrusion_role(erSupportMaterial);
        else if (type == "SoftSupport")
            set_extrusion_role(erSupportMaterialInterface);
        else if (type == "Pillar")
            set_extrusion_role(erWipeTower);
        else {
            set_extrusion_role(erNone);
            assert(false);// BOOST_LOG_TRIVIAL(warning) << "GCodeProcessor found unknown extrusion role: " << type;
        }

        if (m_extrusion_role == erExternalPerimeter)
            m_seams_detector.activate(true);

        return true;
    }

    // layer
    pos = comment.find(" Layer #");
    if (pos == 0) {
        ++m_layer_id;
        return true;
    }

    return false;
}

bool GCodeProcessor::process_ideamaker_tags(const std::string_view comment)
{
    // TYPE -> extrusion role
    std::string tag = "TYPE:";
    size_t pos = comment.find(tag);
    if (pos != comment.npos) {
        const std::string_view type = comment.substr(pos + tag.length());
        if (type == "RAFT")
            set_extrusion_role(erSkirt);
        else if (type == "WALL-OUTER")
            set_extrusion_role(erExternalPerimeter);
        else if (type == "WALL-INNER")
            set_extrusion_role(erPerimeter);
        else if (type == "SOLID-FILL")
            set_extrusion_role(erSolidInfill);
        else if (type == "FILL")
            set_extrusion_role(erInternalInfill);
        else if (type == "BRIDGE")
            set_extrusion_role(erBridgeInfill);
        else if (type == "SUPPORT")
            set_extrusion_role(erSupportMaterial);
        else {
            set_extrusion_role(erNone);
            assert(false); //BOOST_LOG_TRIVIAL(warning) << "GCodeProcessor found unknown extrusion role: " << type;
        }

        if (m_extrusion_role == erExternalPerimeter)
            m_seams_detector.activate(true);

        return true;
    }

    // geometry
    // width
    tag = "WIDTH:";
    pos = comment.find(tag);
    if (pos != comment.npos) {
        if (!parse_number(comment.substr(pos + tag.length()), m_forced_width))
            assert(false); //BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Width (" << comment << ").";
        return true;
    }

    // height
    tag = "HEIGHT:";
    pos = comment.find(tag);
    if (pos != comment.npos) {
        if (!parse_number(comment.substr(pos + tag.length()), m_forced_height))
            assert(false); //BOOST_LOG_TRIVIAL(error) << "GCodeProcessor encountered an invalid value for Height (" << comment << ").";
        return true;
    }

    // layer
    pos = comment.find("LAYER:");
    if (pos == 0) {
        ++m_layer_id;
        return true;
    }

    return false;
}

bool GCodeProcessor::process_kissslicer_tags(const std::string_view comment)
{
    // extrusion roles

    // ; 'Raft Path'
    size_t pos = comment.find(" 'Raft Path'");
    if (pos == 0) {
        set_extrusion_role(erSkirt);
        return true;
    }

    // ; 'Support Interface Path'
    pos = comment.find(" 'Support Interface Path'");
    if (pos == 0) {
        set_extrusion_role(erSupportMaterialInterface);
        return true;
    }

    // ; 'Travel/Ironing Path'
    pos = comment.find(" 'Travel/Ironing Path'");
    if (pos == 0) {
        set_extrusion_role(erIroning);
        return true;
    }

    // ; 'Support (may Stack) Path'
    pos = comment.find(" 'Support (may Stack) Path'");
    if (pos == 0) {
        set_extrusion_role(erSupportMaterial);
        return true;
    }

    // ; 'Perimeter Path'
    pos = comment.find(" 'Perimeter Path'");
    if (pos == 0) {
        set_extrusion_role(erExternalPerimeter);
        m_seams_detector.activate(true);
        return true;
    }

    // ; 'Pillar Path'
    pos = comment.find(" 'Pillar Path'");
    if (pos == 0) {
        set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        return true;
    }

    // ; 'Destring/Wipe/Jump Path'
    pos = comment.find(" 'Destring/Wipe/Jump Path'");
    if (pos == 0) {
        set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        return true;
    }

    // ; 'Prime Pillar Path'
    pos = comment.find(" 'Prime Pillar Path'");
    if (pos == 0) {
        set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        return true;
    }

    // ; 'Loop Path'
    pos = comment.find(" 'Loop Path'");
    if (pos == 0) {
        set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        return true;
    }

    // ; 'Crown Path'
    pos = comment.find(" 'Crown Path'");
    if (pos == 0) {
        set_extrusion_role(erNone); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        return true;
    }

    // ; 'Solid Path'
    pos = comment.find(" 'Solid Path'");
    if (pos == 0) {
        set_extrusion_role(erNone);
        return true;
    }

    // ; 'Stacked Sparse Infill Path'
    pos = comment.find(" 'Stacked Sparse Infill Path'");
    if (pos == 0) {
        set_extrusion_role(erInternalInfill);
        return true;
    }

    // ; 'Sparse Infill Path'
    pos = comment.find(" 'Sparse Infill Path'");
    if (pos == 0) {
        set_extrusion_role(erSolidInfill);
        return true;
    }

    // geometry

    // layer
    pos = comment.find(" BEGIN_LAYER_");
    if (pos == 0) {
        ++m_layer_id;
        return true;
    }

    return false;
}

bool GCodeProcessor::detect_producer(const std::string_view comment)
{
    for (const auto& [id, search_string] : Producers) {
        size_t pos = comment.find(search_string);
        if (pos != comment.npos) {
            m_producer = id;
            //BOOST_LOG_TRIVIAL(info) << "Detected gcode producer: " << search_string;
            return true;
        }
    }
    return false;
}

void GCodeProcessor::process_G0(const GCodeLine& line)
{
    process_G1(line);
}

void GCodeProcessor::process_G1(const GCodeLine& line)
{
    float filament_diameter = (static_cast<size_t>(m_extruder_id) < m_result.filament_diameters.size()) ? m_result.filament_diameters[m_extruder_id] : m_result.filament_diameters.back();
    float filament_radius = 0.5f * filament_diameter;
    float area_filament_cross_section = static_cast<float>(Math::Constant::PI) * sqr(filament_radius);
    auto absolute_position = [this, area_filament_cross_section](Axis axis, const GCodeLine& lineG1) {
        bool is_relative = (m_global_positioning_type == EPositioningType::Relative);
        if (axis == E)
            is_relative |= (m_e_local_positioning_type == EPositioningType::Relative);

        if (lineG1.has(Axis(axis))) {
            float lengthsScaleFactor = (m_units == EUnits::Inches) ? INCHES_TO_MM : 1.0f;
            float ret = lineG1.value(Axis(axis)) * lengthsScaleFactor;
            return is_relative ? m_start_position[axis] + ret : m_origin[axis] + ret;
        }
        else
            return m_start_position[axis];
    };

    auto move_type = [this](const AxisCoords& delta_pos) {
        EMoveType type = EMoveType::Noop;

        if (m_wiping)
            type = EMoveType::Wipe;
        else if (delta_pos[E] < 0.0f)
            type = (delta_pos[X] != 0.0f || delta_pos[Y] != 0.0f || delta_pos[Z] != 0.0f) ? EMoveType::Travel : EMoveType::Retract;
        else if (delta_pos[E] > 0.0f) {
            if (delta_pos[X] == 0.0f && delta_pos[Y] == 0.0f)
                type = (delta_pos[Z] == 0.0f) ? EMoveType::Unretract : EMoveType::Travel;
            else if (delta_pos[X] != 0.0f || delta_pos[Y] != 0.0f)
                type = EMoveType::Extrude;
        }
        else if (delta_pos[X] != 0.0f || delta_pos[Y] != 0.0f || delta_pos[Z] != 0.0f)
            type = EMoveType::Travel;

        return type;
    };

    ++m_g1_line_id;

    // updates axes positions from line
    for (unsigned char a = X; a <= E; ++a) {
        m_end_position[a] = absolute_position((Axis)a, line);
    }

    // updates feedrate from line, if present
    if (line.has_f())
        m_feedrate = line.f() * MMMIN_TO_MMSEC;

    // calculates movement deltas
    float max_abs_delta = 0.0f;
    AxisCoords delta_pos;
    for (unsigned char a = X; a <= E; ++a) {
        delta_pos[a] = m_end_position[a] - m_start_position[a];
        max_abs_delta = std::max<float>(max_abs_delta, std::abs(delta_pos[a]));
    }

    // no displacement, return
    if (max_abs_delta == 0.0f)
        return;

    EMoveType type = move_type(delta_pos);
    if (type == EMoveType::Extrude) {
        float delta_xyz = std::sqrt(sqr(delta_pos[X]) + sqr(delta_pos[Y]) + sqr(delta_pos[Z]));
        float volume_extruded_filament = area_filament_cross_section * delta_pos[E];
        float area_toolpath_cross_section = volume_extruded_filament / delta_xyz;

        // save extruded volume to the cache
        m_used_filaments.increase_caches(volume_extruded_filament);

        // volume extruded filament / tool displacement = area toolpath cross section
        m_mm3_per_mm = area_toolpath_cross_section;

        if (m_forced_height > 0.0f)
            m_height = m_forced_height;
        else {
            if (m_end_position[Z] > m_extruded_last_z + Math::Constant::epsilon)
                m_height = m_end_position[Z] - m_extruded_last_z;
        }

        if (m_height == 0.0f)
            m_height = DEFAULT_TOOLPATH_HEIGHT;

        if (m_end_position[Z] == 0.0f)
            m_end_position[Z] = m_height;

        m_extruded_last_z = m_end_position[Z];
        m_options_z_corrector.update(m_height);

        if (m_forced_width > 0.0f)
            m_width = m_forced_width;
        else if (m_extrusion_role == erExternalPerimeter)
            // cross section: rectangle
            m_width = delta_pos[E] * static_cast<float>(Math::Constant::PI * sqr(1.05f * filament_radius)) / (delta_xyz * m_height);
        else if (m_extrusion_role == erBridgeInfill || m_extrusion_role == erNone)
            // cross section: circle
            m_width = static_cast<float>(m_result.filament_diameters[m_extruder_id]) * std::sqrt(delta_pos[E] / delta_xyz);
        else
            // cross section: rectangle + 2 semicircles
            m_width = delta_pos[E] * static_cast<float>(Math::Constant::PI * sqr(filament_radius)) / (delta_xyz * m_height) + static_cast<float>(1.0 - 0.25 * Math::Constant::PI) * m_height;

        if (m_width == 0.0f)
            m_width = DEFAULT_TOOLPATH_WIDTH;

        // clamp width to avoid artifacts which may arise from wrong values of m_height
        m_width = std::min(m_width, std::max(2.0f, 4.0f * m_height));
    }
    else if (type == EMoveType::Unretract && m_flushing) {
        float volume_flushed_filament = area_filament_cross_section * delta_pos[E];
        if (m_remaining_volume > volume_flushed_filament)
        {
            m_used_filaments.update_flush_per_filament(m_last_extruder_id, volume_flushed_filament);
            m_remaining_volume -= volume_flushed_filament;
        }
        else {
            m_used_filaments.update_flush_per_filament(m_last_extruder_id, m_remaining_volume);
            m_used_filaments.update_flush_per_filament(m_extruder_id, volume_flushed_filament - m_remaining_volume);
            m_remaining_volume = 0.f;
        }
    }

    // time estimate section
    auto move_length = [](const AxisCoords& delta_pos) {
        float sq_xyz_length = sqr(delta_pos[X]) + sqr(delta_pos[Y]) + sqr(delta_pos[Z]);
        return (sq_xyz_length > 0.0f) ? std::sqrt(sq_xyz_length) : std::abs(delta_pos[E]);
    };

    auto is_extrusion_only_move = [](const AxisCoords& delta_pos) {
        return delta_pos[X] == 0.0f && delta_pos[Y] == 0.0f && delta_pos[Z] == 0.0f && delta_pos[E] != 0.0f;
    };

    float distance = move_length(delta_pos);
    assert(distance != 0.0f);
    float inv_distance = 1.0f / distance;

    if (m_seams_detector.is_active()) {
        // check for seam starting vertex
        if (type == EMoveType::Extrude && m_extrusion_role == erExternalPerimeter && !m_seams_detector.has_first_vertex()) {
            //BBS: m_result.moves.back().position has plate offset, must minus plate offset before calculate the real seam position
            const Vec3f real_first_pos = Vec3f(m_result.moves.back().position.x - m_x_offset, m_result.moves.back().position.y - m_y_offset, m_result.moves.back().position.z);
            m_seams_detector.set_first_vertex(real_first_pos - m_extruder_offsets[m_extruder_id]);
        }
        // check for seam ending vertex and store the resulting move
        else if ((type != EMoveType::Extrude || (m_extrusion_role != erExternalPerimeter && m_extrusion_role != erOverhangPerimeter)) && m_seams_detector.has_first_vertex()) {
            auto set_end_position = [this](const Vec3f& pos) {
                m_end_position[X] = pos.x; m_end_position[Y] = pos.y; m_end_position[Z] = pos.z;
            };

            const Vec3f curr_pos(m_end_position[X], m_end_position[Y], m_end_position[Z]);
            //BBS: m_result.moves.back().position has plate offset, must minus plate offset before calculate the real seam position
            const Vec3f real_last_pos = Vec3f(m_result.moves.back().position.x - m_x_offset, m_result.moves.back().position.y - m_y_offset, m_result.moves.back().position.z);
            const Vec3f new_pos = real_last_pos - m_extruder_offsets[m_extruder_id];
            const std::optional<Vec3f> first_vertex = m_seams_detector.get_first_vertex();
            // the threshold value = 0.0625f == 0.25 * 0.25 is arbitrary, we may find some smarter condition later

            if (Math::Dot(new_pos - *first_vertex, new_pos - *first_vertex) < 0.0625f) {
                set_end_position(0.5f * (new_pos + *first_vertex));
                store_move_vertex(EMoveType::Seam);
                set_end_position(curr_pos);
            }

            m_seams_detector.activate(false);
        }
    }
    else if (type == EMoveType::Extrude && m_extrusion_role == erExternalPerimeter) {
        m_seams_detector.activate(true);
        Vec3f plate_offset = { (float)m_x_offset, (float)m_y_offset, 0.0f };
        m_seams_detector.set_first_vertex(m_result.moves.back().position - m_extruder_offsets[m_extruder_id] - plate_offset);
    }

    if (m_spiral_vase_active && !m_result.spiral_vase_layers.empty()) {
        if (m_result.spiral_vase_layers.back().first == FLT_MAX && delta_pos[Z] > 0.0)
            // replace layer height placeholder with correct value
            m_result.spiral_vase_layers.back().first = static_cast<float>(m_end_position[Z]);
        if (!m_result.moves.empty())
            m_result.spiral_vase_layers.back().second.second = m_result.moves.size() - 1;
    }

    // store move
    store_move_vertex(type);
}

// BBS: this function is absolutely new for G2 and G3 gcode
void  GCodeProcessor::process_G2_G3(const GCodeLine& line)
{
    float filament_diameter = (static_cast<size_t>(m_extruder_id) < m_result.filament_diameters.size()) ? m_result.filament_diameters[m_extruder_id] : m_result.filament_diameters.back();
    float filament_radius = 0.5f * filament_diameter;
    float area_filament_cross_section = static_cast<float>(Math::Constant::PI) * sqr(filament_radius);
    auto absolute_position = [this, area_filament_cross_section](Axis axis, const GCodeLine& lineG2_3) {
        bool is_relative = (m_global_positioning_type == EPositioningType::Relative);
        if (axis == E)
            is_relative |= (m_e_local_positioning_type == EPositioningType::Relative);

        if (lineG2_3.has(Axis(axis))) {
            float lengthsScaleFactor = (m_units == EUnits::Inches) ? INCHES_TO_MM : 1.0f;
            float ret = lineG2_3.value(Axis(axis)) * lengthsScaleFactor;
            if (axis == I)
                return m_start_position[X] + ret;
            else if (axis == J)
                return m_start_position[Y] + ret;
            else
                return is_relative ? m_start_position[axis] + ret : m_origin[axis] + ret;
        }
        else {
            if (axis == I)
                return m_start_position[X];
            else if (axis == J)
                return m_start_position[Y];
            else
                return m_start_position[axis];
        }
    };

    auto move_type = [this](const float& delta_E) {
        if (delta_E == 0.0f)
            return EMoveType::Travel;
        else
            return EMoveType::Extrude;
    };

    auto arc_interpolation = [this](const Vec3f& start_pos, const Vec3f& end_pos, const Vec3f& center_pos, const bool is_ccw) {
        float radius = calc_arc_radius(start_pos, center_pos);
        //BBS: radius is too small to draw
        if (radius <= DRAW_ARC_TOLERANCE) {
            m_interpolation_points.resize(0);
            return;
        }
        float radian_step = 2 * acos((radius - DRAW_ARC_TOLERANCE) / radius);
        float num = calc_arc_radian(start_pos, end_pos, center_pos, is_ccw) / radian_step;
        float z_step = (num < 1) ? end_pos.z - start_pos.z : (end_pos.z - start_pos.z) / num;
        radian_step = is_ccw ? radian_step : -radian_step;
        int interpolation_num = floor(num);

        m_interpolation_points.resize(interpolation_num, Vec3f());
        Vec3f delta = start_pos - center_pos;
        for (auto i = 0; i < interpolation_num; i++) {
            float cos_val = cos((i + 1) * radian_step);
            float sin_val = sin((i + 1) * radian_step);
            m_interpolation_points[i] = Vec3f(center_pos.x + delta.x * cos_val - delta.y * sin_val,
                center_pos.y + delta.x * sin_val + delta.y * cos_val,
                start_pos.z + (i + 1) * z_step);
        }
    };

    ++m_g1_line_id;

    //BBS: get axes positions from line
    for (unsigned char a = X; a <= E; ++a) {
        m_end_position[a] = absolute_position((Axis)a, line);
    }
    //BBS: G2 G3 line but has no I and J axis, invalid G code format
    if (!line.has(I) && !line.has(J))
        return;
    //BBS: P mode, but xy position is not same, or P is not 1, invalid G code format
    if (line.has(P) &&
        (m_start_position[X] != m_end_position[X] ||
            m_start_position[Y] != m_end_position[Y] ||
            ((int)line.p()) != 1))
        return;

    m_arc_center = Vec3f(absolute_position(I, line), absolute_position(J, line), m_start_position[Z]);
    //BBS: G2 is CW direction, G3 is CCW direction
    const std::string_view cmd = line.cmd();
    m_move_path_type = (::atoi(&cmd[1]) == 2) ? EMovePathType::Arc_move_cw : EMovePathType::Arc_move_ccw;
    //BBS: get arc length,interpolation points and radian in X-Y plane
    Vec3f start_point = Vec3f(m_start_position[X], m_start_position[Y], m_start_position[Z]);
    Vec3f end_point = Vec3f(m_end_position[X], m_end_position[Y], m_end_position[Z]);
    float arc_length;
    if (!line.has(P))
        arc_length = calc_arc_length(start_point, end_point, m_arc_center, (m_move_path_type == EMovePathType::Arc_move_ccw));
    else
        arc_length = ((int)line.p()) * 2 * Math::Constant::PI * Math::Length(start_point - m_arc_center);
    //BBS: Attention! arc_onterpolation does not support P mode while P is not 1.
    arc_interpolation(start_point, end_point, m_arc_center, (m_move_path_type == EMovePathType::Arc_move_ccw));
    float radian = calc_arc_radian(start_point, end_point, m_arc_center, (m_move_path_type == EMovePathType::Arc_move_ccw));
    Vec3f start_dir = calc_tangential_vector(start_point, m_arc_center, (m_move_path_type == EMovePathType::Arc_move_ccw));
    Vec3f end_dir = calc_tangential_vector(end_point, m_arc_center, (m_move_path_type == EMovePathType::Arc_move_ccw));

    //BBS: updates feedrate from line, if present
    if (line.has_f())
        m_feedrate = line.f() * MMMIN_TO_MMSEC;

    //BBS: calculates movement deltas
    AxisCoords delta_pos;
    for (unsigned char a = X; a <= E; ++a) {
        delta_pos[a] = m_end_position[a] - m_start_position[a];
    }

    //BBS: no displacement, return
    if (arc_length == 0.0f && delta_pos[Z] == 0.0f)
        return;

    EMoveType type = move_type(delta_pos[E]);


    float delta_xyz = std::sqrt(sqr(arc_length) + sqr(delta_pos[Z]));
    if (type == EMoveType::Extrude) {
        float volume_extruded_filament = area_filament_cross_section * delta_pos[E];
        float area_toolpath_cross_section = volume_extruded_filament / delta_xyz;

        //BBS: save extruded volume to the cache
        m_used_filaments.increase_caches(volume_extruded_filament);

        //BBS: volume extruded filament / tool displacement = area toolpath cross section
        m_mm3_per_mm = area_toolpath_cross_section;

        if (m_forced_height > 0.0f)
            m_height = m_forced_height;
        else {
            if (m_end_position[Z] > m_extruded_last_z + Math::Constant::epsilon)
                m_height = m_end_position[Z] - m_extruded_last_z;
        }

        if (m_height == 0.0f)
            m_height = DEFAULT_TOOLPATH_HEIGHT;

        if (m_end_position[Z] == 0.0f)
            m_end_position[Z] = m_height;

        m_extruded_last_z = m_end_position[Z];
        m_options_z_corrector.update(m_height);

        if (m_forced_width > 0.0f)
            m_width = m_forced_width;
        else if (m_extrusion_role == erExternalPerimeter)
            //BBS: cross section: rectangle
            m_width = delta_pos[E] * static_cast<float>(Math::Constant::PI * sqr(1.05f * filament_radius)) / (delta_xyz * m_height);
        else if (m_extrusion_role == erBridgeInfill || m_extrusion_role == erNone)
            //BBS: cross section: circle
            m_width = static_cast<float>(m_result.filament_diameters[m_extruder_id]) * std::sqrt(delta_pos[E] / delta_xyz);
        else
            //BBS: cross section: rectangle + 2 semicircles
            m_width = delta_pos[E] * static_cast<float>(Math::Constant::PI * sqr(filament_radius)) / (delta_xyz * m_height) + static_cast<float>(1.0 - 0.25 * Math::Constant::PI) * m_height;

        if (m_width == 0.0f)
            m_width = DEFAULT_TOOLPATH_WIDTH;

        //BBS: clamp width to avoid artifacts which may arise from wrong values of m_height
        m_width = std::min(m_width, std::max(2.0f, 4.0f * m_height));
    }

    //BBS: time estimate section
    assert(delta_xyz != 0.0f);
    float inv_distance = 1.0f / delta_xyz;
    float radius = calc_arc_radius(start_point, m_arc_center);

    //BBS: seam detector
    Vec3f plate_offset = { (float)m_x_offset, (float)m_y_offset, 0.0f };

    if (m_seams_detector.is_active()) {
        //BBS: check for seam starting vertex
        if (type == EMoveType::Extrude && m_extrusion_role == erExternalPerimeter && !m_seams_detector.has_first_vertex()) {
            m_seams_detector.set_first_vertex(m_result.moves.back().position - m_extruder_offsets[m_extruder_id] - plate_offset);
        }
        //BBS: check for seam ending vertex and store the resulting move
        else if ((type != EMoveType::Extrude || (m_extrusion_role != erExternalPerimeter && m_extrusion_role != erOverhangPerimeter)) && m_seams_detector.has_first_vertex()) {
            auto set_end_position = [this](const Vec3f& pos) {
                m_end_position[X] = pos.x; m_end_position[Y] = pos.y; m_end_position[Z] = pos.z;
            };
            const Vec3f curr_pos(m_end_position[X], m_end_position[Y], m_end_position[Z]);
            const Vec3f new_pos = m_result.moves.back().position - m_extruder_offsets[m_extruder_id] - plate_offset;
            const std::optional<Vec3f> first_vertex = m_seams_detector.get_first_vertex();
            //BBS: the threshold value = 0.0625f == 0.25 * 0.25 is arbitrary, we may find some smarter condition later

            if (Math::Dot(new_pos - *first_vertex, new_pos - *first_vertex) < 0.0625f) {
                set_end_position(0.5f * (new_pos + *first_vertex));
                store_move_vertex(EMoveType::Seam);
                set_end_position(curr_pos);
            }

            m_seams_detector.activate(false);
        }
    }
    else if (type == EMoveType::Extrude && m_extrusion_role == erExternalPerimeter) {
        m_seams_detector.activate(true);
        m_seams_detector.set_first_vertex(m_result.moves.back().position - m_extruder_offsets[m_extruder_id] - plate_offset);
    }
    //BBS: store move
    store_move_vertex(type, m_move_path_type);
}

//BBS
void GCodeProcessor::process_G4(const GCodeLine& line)
{

}

//BBS
void GCodeProcessor::process_G29(const GCodeLine& line)
{

}

void GCodeProcessor::process_G10(const GCodeLine& line)
{
    // stores retract move
    store_move_vertex(EMoveType::Retract);
}

void GCodeProcessor::process_G11(const GCodeLine& line)
{
    // stores unretract move
    store_move_vertex(EMoveType::Unretract);
}

void GCodeProcessor::process_G20(const GCodeLine& line)
{
    m_units = EUnits::Inches;
}

void GCodeProcessor::process_G21(const GCodeLine& line)
{
    m_units = EUnits::Millimeters;
}

void GCodeProcessor::process_G22(const GCodeLine& line)
{
    // stores retract move
    store_move_vertex(EMoveType::Retract);
}

void GCodeProcessor::process_G23(const GCodeLine& line)
{
    // stores unretract move
    store_move_vertex(EMoveType::Unretract);
}

void GCodeProcessor::process_G28(const GCodeLine& line)
{
    std::string_view cmd = line.cmd();
    std::string new_line_raw = { cmd.data(), cmd.size() };
    bool found = false;
    if (line.has('X')) {
        new_line_raw += " X0";
        found = true;
    }
    if (line.has('Y')) {
        new_line_raw += " Y0";
        found = true;
    }
    if (line.has('Z')) {
        new_line_raw += " Z0";
        found = true;
    }
    if (!found)
        new_line_raw += " X0  Y0  Z0";

    GCodeLine new_gline;
    GCodeReader reader;
    reader.parse_line(new_line_raw, [&](GCodeReader& reader, const GCodeLine& gline) { new_gline = gline; });
    process_G1(new_gline);
}

void GCodeProcessor::process_G90(const GCodeLine& line)
{
    m_global_positioning_type = EPositioningType::Absolute;
}

void GCodeProcessor::process_G91(const GCodeLine& line)
{
    m_global_positioning_type = EPositioningType::Relative;
}

void GCodeProcessor::process_G92(const GCodeLine& line)
{
    float lengths_scale_factor = (m_units == EUnits::Inches) ? INCHES_TO_MM : 1.0f;
    bool any_found = false;

    if (line.has_x()) {
        m_origin[X] = m_end_position[X] - line.x() * lengths_scale_factor;
        any_found = true;
    }

    if (line.has_y()) {
        m_origin[Y] = m_end_position[Y] - line.y() * lengths_scale_factor;
        any_found = true;
    }

    if (line.has_z()) {
        m_origin[Z] = m_end_position[Z] - line.z() * lengths_scale_factor;
        any_found = true;
    }

    if (line.has_e()) {
        // extruder coordinate can grow to the point where its float representation does not allow for proper addition with small increments,
        // we set the value taken from the G92 line as the new current position for it
        m_end_position[E] = line.e() * lengths_scale_factor;
        any_found = true;
    }

    if (!any_found && !line.has_unknown_axis()) {
        // The G92 may be called for axes that PrusaSlicer does not recognize, for example see GH issue #3510,
        // where G92 A0 B0 is called although the extruder axis is till E.
        for (unsigned char a = X; a <= E; ++a) {
            m_origin[a] = m_end_position[a];
        }
    }
}

void GCodeProcessor::process_M1(const GCodeLine& line)
{

}

void GCodeProcessor::process_M82(const GCodeLine& line)
{
    m_e_local_positioning_type = EPositioningType::Absolute;
}

void GCodeProcessor::process_M83(const GCodeLine& line)
{
    m_e_local_positioning_type = EPositioningType::Relative;
}

void GCodeProcessor::process_M104(const GCodeLine& line)
{
    float new_temp;
    if (line.has_value('S', new_temp))
        m_extruder_temps[m_extruder_id] = new_temp;
}

void GCodeProcessor::process_M106(const GCodeLine& line)
{
    //BBS: for Bambu machine ,we both use M106 P1 and M106 to indicate the part cooling fan
    //So we must not ignore M106 P1
    if (!line.has('P') || (line.has('P') && line.p() == 1.0f)) {
        // The absence of P means the print cooling fan, so ignore anything else.
        float new_fan_speed;
        if (line.has_value('S', new_fan_speed))
            m_fan_speed = (100.0f / 255.0f) * new_fan_speed;
        else
            m_fan_speed = 100.0f;
    }
}

void GCodeProcessor::process_M107(const GCodeLine& line)
{
    m_fan_speed = 0.0f;
}

void GCodeProcessor::process_M108(const GCodeLine& line)
{
    // These M-codes are used by Sailfish to change active tool.
    // They have to be processed otherwise toolchanges will be unrecognised

    if (m_flavor != gcfSailfish)
        return;

    std::string cmd = line.raw();
    size_t pos = cmd.find("T");
    if (pos != std::string::npos)
        process_T(cmd.substr(pos));
}

void GCodeProcessor::process_M109(const GCodeLine& line)
{
    float new_temp;
    if (line.has_value('R', new_temp)) {
        float val;
        if (line.has_value('T', val)) {
            size_t eid = static_cast<size_t>(val);
            if (eid < m_extruder_temps.size())
                m_extruder_temps[eid] = new_temp;
        }
        else
            m_extruder_temps[m_extruder_id] = new_temp;
    }
    else if (line.has_value('S', new_temp))
        m_extruder_temps[m_extruder_id] = new_temp;
}

void GCodeProcessor::process_M132(const GCodeLine& line)
{
    // This command is used by Makerbot to load the current home position from EEPROM
    // see: https://github.com/makerbot/s3g/blob/master/doc/GCodeProtocol.md

    if (line.has('X'))
        m_origin[X] = 0.0f;

    if (line.has('Y'))
        m_origin[Y] = 0.0f;

    if (line.has('Z'))
        m_origin[Z] = 0.0f;

    if (line.has('E'))
        m_origin[E] = 0.0f;
}

void GCodeProcessor::process_M135(const GCodeLine& line)
{
    // These M-codes are used by MakerWare to change active tool.
    // They have to be processed otherwise toolchanges will be unrecognised

    if (m_flavor != gcfMakerWare)
        return;

    std::string cmd = line.raw();
    size_t pos = cmd.find("T");
    if (pos != std::string::npos)
        process_T(cmd.substr(pos));
}

void GCodeProcessor::process_M140(const GCodeLine& line)
{
    float new_temp;
    if (line.has_value('S', new_temp))
        m_highest_bed_temp = m_highest_bed_temp < (int)new_temp ? (int)new_temp : m_highest_bed_temp;
}

void GCodeProcessor::process_M190(const GCodeLine& line)
{
    float new_temp;
    if (line.has_value('S', new_temp))
        m_highest_bed_temp = m_highest_bed_temp < (int)new_temp ? (int)new_temp : m_highest_bed_temp;
}


void GCodeProcessor::process_M201(const GCodeLine& line)
{

}

void GCodeProcessor::process_M203(const GCodeLine& line)
{

}

void GCodeProcessor::process_M204(const GCodeLine& line)
{

}

void GCodeProcessor::process_M205(const GCodeLine& line)
{

}

void GCodeProcessor::process_SET_VELOCITY_LIMIT(const GCodeLine& line)
{

}

void GCodeProcessor::process_M221(const GCodeLine& line)
{

}

void GCodeProcessor::process_M400(const GCodeLine& line)
{

}

void GCodeProcessor::process_M401(const GCodeLine& line)
{
    if (m_flavor != gcfRepetier)
        return;

    for (unsigned char a = 0; a <= 3; ++a) {
        m_cached_position.position[a] = m_start_position[a];
    }
    m_cached_position.feedrate = m_feedrate;
}

void GCodeProcessor::process_M402(const GCodeLine& line)
{
    if (m_flavor != gcfRepetier)
        return;

    // see for reference:
    // https://github.com/repetier/Repetier-Firmware/blob/master/src/ArduinoAVR/Repetier/Printer.cpp
    // void Printer::GoToMemoryPosition(bool x, bool y, bool z, bool e, float feed)

    bool has_xyz = !(line.has('X') || line.has('Y') || line.has('Z'));

    float p = FLT_MAX;
    for (unsigned char a = X; a <= Z; ++a) {
        if (has_xyz || line.has(a)) {
            p = m_cached_position.position[a];
            if (p != FLT_MAX)
                m_start_position[a] = p;
        }
    }

    p = m_cached_position.position[E];
    if (p != FLT_MAX)
        m_start_position[E] = p;

    p = FLT_MAX;
    if (!line.has_value(4, p))
        p = m_cached_position.feedrate;

    if (p != FLT_MAX)
        m_feedrate = p;
}

void GCodeProcessor::process_M566(const GCodeLine& line)
{

}

void GCodeProcessor::process_M702(const GCodeLine& line)
{

}

void GCodeProcessor::process_T(const GCodeLine& line)
{
    process_T(line.cmd());
}

void GCodeProcessor::process_T(const std::string_view command)
{
    if (command.length() > 1) {
        int eid = 0;
        if (!parse_number(command.substr(1), eid) || eid < 0 || eid > 254) {
            //BBS: T255, T1000 and T1100 is used as special command for BBL machine and does not cost time. return directly
            if ((m_flavor == gcfMarlinLegacy || m_flavor == gcfMarlinFirmware) && (command == "Tx" || command == "Tc" || command == "T?" ||
                eid == 1000 || eid == 1100 || eid == 255))
                return;

            // T-1 is a valid gcode line for RepRap Firmwares (used to deselects all tools)
            if ((m_flavor != gcfRepRapFirmware && m_flavor != gcfRepRapSprinter) || eid != -1)
                ;// assert(false); //BOOST_LOG_TRIVIAL(error) << "Invalid T command (" << command << ").";
        }
        else {
            unsigned char id = static_cast<unsigned char>(eid);
            if (m_extruder_id != id) {
                if (id >= m_result.extruders_count)
                    assert(false); //BOOST_LOG_TRIVIAL(error) << "Invalid T command (" << command << ").";
                else {
                    m_last_extruder_id = m_extruder_id;
                    process_filaments(CustomGCode::ToolChange);
                    m_extruder_id = id;
                    m_cp_color.current = m_extruder_colors[id];
                    //BBS: increase filament change times
                    //m_result.lock();
                    m_result.print_statistics.total_filamentchanges++;
                    //m_result.unlock();

                    // Specific to the MK3 MMU2:
                    // The initial value of extruder_unloaded is set to true indicating
                    // that the filament is parked in the MMU2 unit and there is nothing to be unloaded yet.
                    //float extra_time = get_filament_unload_time(static_cast<size_t>(m_last_extruder_id));
                    //extra_time += get_filament_load_time(static_cast<size_t>(m_extruder_id));
                    //simulate_st_synchronize(extra_time);
                }

                // store tool change move
                store_move_vertex(EMoveType::Tool_change);
            }
        }
    }
}

void GCodeProcessor::store_move_vertex(EMoveType type, EMovePathType path_type)
{
    m_last_line_id = (type == EMoveType::Color_change || type == EMoveType::Pause_Print || type == EMoveType::Custom_GCode) ?
        m_line_id + 1 :
        ((type == EMoveType::Seam) ? m_last_line_id : m_line_id);

    //BBS: apply plate's and extruder's offset to arc interpolation points
    if (path_type == EMovePathType::Arc_move_cw ||
        path_type == EMovePathType::Arc_move_ccw) {
        for (size_t i = 0; i < m_interpolation_points.size(); i++)
            m_interpolation_points[i] =
            Vec3f(m_interpolation_points[i].x + m_x_offset,
                m_interpolation_points[i].y + m_y_offset,
                m_processing_start_custom_gcode ? m_first_layer_height : m_interpolation_points[i].z) +
            m_extruder_offsets[m_extruder_id];
    }

    m_result.moves.push_back({
        m_last_line_id,
        type,
        m_extrusion_role,
        m_extruder_id,
        m_cp_color.current,
        //BBS: add plate's offset to the rendering vertices
        Vec3f(m_end_position[X] + m_x_offset, m_end_position[Y] + m_y_offset, m_processing_start_custom_gcode ? m_first_layer_height : m_end_position[Z]) + m_extruder_offsets[m_extruder_id],
        static_cast<float>(m_end_position[E] - m_start_position[E]),
        m_feedrate,
        m_width,
        m_height,
        m_mm3_per_mm,
        m_fan_speed,
        m_extruder_temps[m_extruder_id],
        static_cast<float>(m_result.moves.size()),
        static_cast<float>(m_layer_id), //layer_duration: set later
        //BBS: add arc move related data
        path_type,
        Vec3f(m_arc_center.x + m_x_offset, m_arc_center.y + m_y_offset, m_arc_center.z) + m_extruder_offsets[m_extruder_id],
        m_interpolation_points,
        });
}

void GCodeProcessor::set_extrusion_role(ExtrusionRole role)
{
    m_used_filaments.process_role_cache(this);
    m_extrusion_role = role;
}

//BBS
int GCodeProcessor::get_filament_vitrification_temperature(size_t extrude_id)
{
    if (extrude_id < m_result.filament_vitrification_temperature.size())
        return m_result.filament_vitrification_temperature[extrude_id];
    else
        return 0;
}

void GCodeProcessor::process_custom_gcode_time(CustomGCode::Type code)
{

}

void GCodeProcessor::process_filaments(CustomGCode::Type code)
{
    if (code == CustomGCode::ColorChange)
        m_used_filaments.process_color_change_cache();

    if (code == CustomGCode::ToolChange) {
        m_used_filaments.process_extruder_cache(this);
        //BBS: reset remaining filament
        m_remaining_volume = m_nozzle_volume;
    }
}

void GCodeProcessor::update_estimated_times_stats()
{
    //auto update_mode = [this](PrintEstimatedStatistics::ETimeMode mode) {
    //    PrintEstimatedStatistics::Mode& data = m_result.print_statistics.modes[static_cast<size_t>(mode)];
    //    data.time = get_time(mode);
    //    data.prepare_time = get_prepare_time(mode);
    //    data.custom_gcode_times = get_custom_gcode_times(mode, true);
    //    data.moves_times = get_moves_time(mode);
    //    data.roles_times = get_roles_time(mode);
    //    data.layers_times = get_layers_time(mode);
    //};
    //update_mode(PrintEstimatedStatistics::ETimeMode::Normal);
    //if (m_time_processor.machines[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].enabled)
    //    update_mode(PrintEstimatedStatistics::ETimeMode::Stealth);
    //else
        m_result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].reset();

    m_result.print_statistics.volumes_per_color_change = m_used_filaments.volumes_per_color_change;
    m_result.print_statistics.volumes_per_extruder = m_used_filaments.volumes_per_extruder;
    m_result.print_statistics.flush_per_filament = m_used_filaments.flush_per_filament;
    m_result.print_statistics.used_filaments_per_role = m_used_filaments.filaments_per_role;
}

//BBS: ugly code...
void GCodeProcessor::update_slice_warnings()
{
    m_result.warnings.clear();

    auto get_used_extruders = [this]() {
        std::vector<size_t> used_extruders;
        used_extruders.reserve(m_used_filaments.volumes_per_extruder.size());
        for (auto item : m_used_filaments.volumes_per_extruder) {
            used_extruders.push_back(item.first);
        }
        return used_extruders;
    };

    auto used_extruders = get_used_extruders();
    assert(!used_extruders.empty());
    GCodeProcessorResult::SliceWarning warning;
    warning.level = 1;
    if (m_highest_bed_temp != 0) {
        for (size_t i = 0; i < used_extruders.size(); i++) {
            int temperature = get_filament_vitrification_temperature(used_extruders[i]);
            if (temperature != 0 && m_highest_bed_temp > temperature)
                warning.params.push_back(std::to_string(used_extruders[i]));
        }
    }

    if (!warning.params.empty()) {
        warning.msg = BED_TEMP_TOO_HIGH_THAN_FILAMENT;
        warning.error_code = "1000C001";
        m_result.warnings.push_back(warning);
    }

    //bbs:HRC checker
    warning.params.clear();
    warning.level = 1;
    if (m_result.nozzle_hrc != 0) {
        for (size_t i = 0; i < used_extruders.size(); i++) {
            int HRC = 0;
            if (used_extruders[i] < m_result.required_nozzle_HRC.size())
                HRC = m_result.required_nozzle_HRC[used_extruders[i]];
            if (HRC != 0 && (m_result.nozzle_hrc < HRC))
                warning.params.push_back(std::to_string(used_extruders[i]));
        }
    }

    if (!warning.params.empty()) {
        warning.msg = NOZZLE_HRC_CHECKER;
        warning.error_code = "1000C002";
        m_result.warnings.push_back(warning);
    }

    m_result.warnings.shrink_to_fit();
}