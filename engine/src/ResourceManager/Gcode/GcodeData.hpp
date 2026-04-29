#ifndef GcodeData_hpp
#define GcodeData_hpp

#include "Base/Common.hpp"
#include "fast_float/fast_float.h"
#include <charconv>

using Vec3f = Vec3;
using Vec2f = Vec2;
using Pointfs = std::vector<Vec2>;
using AxisCoords = std::array<double, 4>;

class GCodeProcessor;

static inline const std::vector<std::string> Reserved_Tags = {
    " FEATURE: ",
    " WIPE_START",
    " WIPE_END",
    " LAYER_HEIGHT: ",
    " LINE_WIDTH: ",
    " CHANGE_LAYER",
    " COLOR_CHANGE",
    " PAUSE_PRINTING",
    " CUSTOM_GCODE",
    "_GP_FIRST_LINE_M73_PLACEHOLDER",
    "_GP_LAST_LINE_M73_PLACEHOLDER",
    "_GP_ESTIMATED_PRINTING_TIME_PLACEHOLDER",
    "_GP_TOTAL_LAYER_NUMBER_PLACEHOLDER"};

enum class ETags : unsigned char
{
    Role,
    Wipe_Start,
    Wipe_End,
    Height,
    Width,
    Layer_Change,
    Color_Change,
    Pause_Print,
    Custom_Code,
    First_Line_M73_Placeholder,
    Last_Line_M73_Placeholder,
    Estimated_Printing_Time_Placeholder,
    Total_Layer_Number_Placeholder
};

enum GCodeFlavor : unsigned char
{
    gcfMarlinLegacy,
    gcfKlipper,
    gcfRepRapSprinter,
    gcfRepRapFirmware,
    gcfRepetier,
    gcfTeacup,
    gcfMakerWare,
    gcfMarlinFirmware,
    gcfSailfish,
    gcfMach3,
    gcfMachinekit,
    gcfSmoothie,
    gcfNoExtrusion
};

namespace CustomGCode
{
    enum Type
    {
        ColorChange,
        PausePrint,
        ToolChange,
        Template,
        Custom,
        Unknown,
    };

    struct Item
    {
        bool operator<(const Item &rhs) const { return this->print_z < rhs.print_z; }
        bool operator==(const Item &rhs) const
        {
            return (rhs.print_z == this->print_z) &&
                   (rhs.type == this->type) &&
                   (rhs.extruder == this->extruder) &&
                   (rhs.color == this->color) &&
                   (rhs.extra == this->extra);
        }
        bool operator!=(const Item &rhs) const { return !(*this == rhs); }

        double print_z;
        Type type;
        int extruder; // Informative value for ColorChangeCode and ToolChangeCode
        // "gcode" == ColorChangeCode   => M600 will be applied for "extruder" extruder
        // "gcode" == ToolChangeCode    => for whole print tool will be switched to "extruder" extruder
        std::string color; // if gcode is equal to PausePrintCode,
        // this field is used for save a short message shown on Printer display
        std::string extra; // this field is used for the extra data like :
        // - G-code text for the Type::Custom
        // - message text for the Type::PausePrint
    };
}

enum Axis
{
    X = 0,
    Y,
    Z,
    E,
    F,
    // BBS: add I, J, P axis
    I,
    J,
    P,
    NUM_AXES,
    // For the GCodeReader to mark a parsed axis, which is not in "XYZEF", it was parsed correctly.
    UNKNOWN_AXIS = NUM_AXES,
    NUM_AXES_WITH_UNKNOWN,
};

enum class EUnits : unsigned char
{
    Millimeters,
    Inches
};

enum class EPositioningType : unsigned char
{
    Absolute,
    Relative
};

enum class EProducer
{
    Unknown,
    BambuStudio,
    Slic3rPE,
    Slic3r,
    SuperSlicer,
    Cura,
    Simplify3D,
    CraftWare,
    ideaMaker,
    KissSlicer
};

enum EMoveType : uint8_t
{
    Noop,
    Retract,
    Unretract,
    Seam,
    Tool_change,
    Color_change,
    Pause_Print,
    Custom_GCode,
    Travel,
    Wipe,
    Extrude,
    Count
};

enum ExtrusionRole : uint8_t
{
    erNone,
    erPerimeter,
    erExternalPerimeter,
    erOverhangPerimeter,
    erInternalInfill,
    erSolidInfill,
    erTopSolidInfill,
    erBottomSurface,
    erIroning,
    erBridgeInfill,
    erGapFill,
    erSkirt,
    erBrim,
    erSupportMaterial,
    erSupportMaterialInterface,
    erSupportTransition,
    erWipeTower,
    erCustom,
    // Extrusion role for a collection with multiple extrusion roles.
    erMixed,
    erCount
};

enum class EMovePathType : unsigned char
{
    Noop_move,
    Linear_move,
    Arc_move_cw,
    Arc_move_ccw,
    Count
};

enum NozzleType
{
    ntUndefine = 0,
    ntHardenedSteel,
    ntStainlessSteel,
    ntBrass,
    ntCount
};

enum BedType
{
    btDefault = 0,
    btPC,
    btEP,
    btPEI,
    btPTE,
    btCount
};

static inline const float DEFAULT_TOOLPATH_WIDTH = 0.4f;
static inline const float DEFAULT_TOOLPATH_HEIGHT = 0.2f;

static inline const float INCHES_TO_MM = 25.4f;
static inline const float MMMIN_TO_MMSEC = 1.0f / 60.0f;
static inline const float DRAW_ARC_TOLERANCE = 0.0125f; // 0.0125mm tolerance for drawing arc

static inline const float DEFAULT_ACCELERATION = 1500.0f;         // Prusa Firmware 1_75mm_MK2
static inline const float DEFAULT_RETRACT_ACCELERATION = 1500.0f; // Prusa Firmware 1_75mm_MK2
static inline const float DEFAULT_TRAVEL_ACCELERATION = 1250.0f;

static inline const size_t MIN_EXTRUDERS_COUNT = 5;
static inline const float DEFAULT_FILAMENT_DIAMETER = 1.75f;
static inline const int DEFAULT_FILAMENT_HRC = 0;
static inline const float DEFAULT_FILAMENT_DENSITY = 1.245f;
static inline const int DEFAULT_FILAMENT_VITRIFICATION_TEMPERATURE = 0;
static inline const Vec3f DEFAULT_EXTRUDER_OFFSET = Vec3f();

static inline const std::string NOZZLE_HRC_CHECKER = "the_actual_nozzle_hrc_smaller_than_the_required_nozzle_hrc";
static inline const std::string BED_TEMP_TOO_HIGH_THAN_FILAMENT = "bed_temperature_too_high_than_filament";

static inline bool is_whitespace(char c) { return c == ' ' || c == '\t'; }
static inline bool is_end_of_line(char c) { return c == '\r' || c == '\n' || c == 0; }
static inline bool is_end_of_gcode_line(char c) { return c == ';' || is_end_of_line(c); }
static inline bool is_end_of_word(char c) { return is_whitespace(c) || is_end_of_gcode_line(c); }
static inline const char *skip_whitespaces(const char *c)
{
    for (; is_whitespace(*c); ++c)
        ;
    return c;
}
static inline const char *skip_word(const char *c)
{
    for (; !is_end_of_word(*c); ++c)
        ;
    return c;
}
static inline bool is_decimal_separator_point()
{
    char str[5] = "";
    sprintf(str, "%.1f", 0.5f);
    return str[1] == '.';
}
static inline const std::string &reserved_tag(ETags tag) { return Reserved_Tags[static_cast<unsigned char>(tag)]; }
static inline ExtrusionRole string_to_extrusion_role(const std::string_view role)
{
    if (role == ("Inner wall"))
        return erPerimeter;
    else if (role == ("Outer wall"))
        return erExternalPerimeter;
    else if (role == ("Overhang wall"))
        return erOverhangPerimeter;
    else if (role == ("Sparse infill"))
        return erInternalInfill;
    else if (role == ("Internal solid infill"))
        return erSolidInfill;
    else if (role == ("Top surface"))
        return erTopSolidInfill;
    else if (role == ("Bottom surface"))
        return erBottomSurface;
    else if (role == ("Ironing"))
        return erIroning;
    else if (role == ("Bridge"))
        return erBridgeInfill;
    else if (role == ("Gap infill"))
        return erGapFill;
    else if (role == ("Skirt"))
        return erSkirt;
    else if (role == ("Brim"))
        return erBrim;
    else if (role == ("Support"))
        return erSupportMaterial;
    else if (role == ("Support interface"))
        return erSupportMaterialInterface;
    else if (role == ("Support transition"))
        return erSupportTransition;
    else if (role == ("Prime tower"))
        return erWipeTower;
    else if (role == ("Custom"))
        return erCustom;
    else if (role == ("Multiple"))
        return erMixed;
    else
        return erNone;
}
static inline std::string get_time_dhms(float time_in_secs)
{
    int days = (int)(time_in_secs / 86400.0f);
    time_in_secs -= (float)days * 86400.0f;
    int hours = (int)(time_in_secs / 3600.0f);
    time_in_secs -= (float)hours * 3600.0f;
    int minutes = (int)(time_in_secs / 60.0f);
    time_in_secs -= (float)minutes * 60.0f;

    char buffer[64];
    if (days > 0)
        ::sprintf(buffer, "%dd %dh %dm %ds", days, hours, minutes, (int)time_in_secs);
    else if (hours > 0)
        ::sprintf(buffer, "%dh %dm %ds", hours, minutes, (int)time_in_secs);
    else if (minutes > 0)
        ::sprintf(buffer, "%dm %ds", minutes, (int)time_in_secs);
    else if (time_in_secs > 1)
        ::sprintf(buffer, "%ds", (int)time_in_secs);
    else
        ::sprintf(buffer, "%fs", time_in_secs);

    return buffer;
}
static inline std::string short_time(const std::string &time)
{
    // Parse the dhms time format.
    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    float f_seconds = 0.0;
    if (time.find('d') != std::string::npos)
        ::sscanf(time.c_str(), "%dd %dh %dm %ds", &days, &hours, &minutes, &seconds);
    else if (time.find('h') != std::string::npos)
        ::sscanf(time.c_str(), "%dh %dm %ds", &hours, &minutes, &seconds);
    else if (time.find('m') != std::string::npos)
        ::sscanf(time.c_str(), "%dm %ds", &minutes, &seconds);
    else if (time.find('s') != std::string::npos)
    {
        ::sscanf(time.c_str(), "%fs", &f_seconds);
        seconds = int(f_seconds);
    }
    // Round to full minutes.
    if (days + hours > 0 && seconds >= 30)
    {
        if (++minutes == 60)
        {
            minutes = 0;
            if (++hours == 24)
            {
                hours = 0;
                ++days;
            }
        }
    }
    // Format the dhm time.
    char buffer[64];
    if (days > 0)
        ::sprintf(buffer, "%dd%dh%dm", days, hours, minutes);
    else if (hours > 0)
        ::sprintf(buffer, "%dh%dm", hours, minutes);
    else if (minutes > 0)
        ::sprintf(buffer, "%dm%ds", minutes, (int)seconds);
    else if (seconds >= 1)
        ::sprintf(buffer, "%ds", (int)seconds);
    else if (f_seconds > 0 && f_seconds < 1)
        ::sprintf(buffer, "<1s");
    else if (seconds == 0)
        ::sprintf(buffer, "0s");
    return buffer;
}
static inline std::string float_to_string_decimal_point(double value, int precision = -1)
{
    // Our Windows build server fully supports C++17 std::to_chars. Let's use it.
    // Other platforms are behind, fall back to slow stringstreams for now.
#ifdef _WIN32
    constexpr size_t SIZE = 20;
    char out[SIZE] = "";
    std::to_chars_result res;
    if (precision >= 0)
        res = std::to_chars(out, out + SIZE, value, std::chars_format::fixed, precision);
    else
        res = std::to_chars(out, out + SIZE, value, std::chars_format::general, 6);
    if (res.ec == std::errc::value_too_large)
        throw std::invalid_argument("float_to_string_decimal_point conversion failed.");
    return std::string(out, res.ptr - out);
#else
    std::stringstream buf;
    if (precision >= 0)
        buf << std::fixed << std::setprecision(precision);
    buf << value;
    return buf.str();
#endif
}
static inline double string_to_double_decimal_point(const std::string_view str, size_t *pos = nullptr)
{
    double out;
    size_t p = fast_float::from_chars(str.data(), str.data() + str.size(), out).ptr - str.data();
    if (pos)
        *pos = p;
    return out;
}

template <typename T>
constexpr inline T sqr(T x) { return x * x; }

template <class T, class O = T>
using IntegerOnly = std::enable_if_t<std::is_integral<T>::value, O>;
template <class T, class I, class... Args> // Arbitrary allocator can be used
IntegerOnly<I, std::vector<T, Args...>> reserve_vector(I capacity)
{
    std::vector<T, Args...> ret;
    if (capacity > I(0))
        ret.reserve(size_t(capacity));

    return ret;
}

static inline float estimated_acceleration_distance(float initial_rate, float target_rate, float acceleration)
{
    return (acceleration == 0.0f) ? 0.0f : (sqr(target_rate) - sqr(initial_rate)) / (2.0f * acceleration);
}

static inline float intersection_distance(float initial_rate, float final_rate, float acceleration, float distance)
{
    return (acceleration == 0.0f) ? 0.0f : (2.0f * acceleration * distance - sqr(initial_rate) + sqr(final_rate)) / (4.0f * acceleration);
}

static inline float speed_from_distance(float initial_feedrate, float distance, float acceleration)
{
    // to avoid invalid negative numbers due to numerical errors
    float value = std::max(0.0f, sqr(initial_feedrate) + 2.0f * acceleration * distance);
    return ::sqrt(value);
}

// Calculates the maximum allowable speed at this point when you must be able to reach target_velocity using the
// acceleration within the allotted distance.
static inline float max_allowable_speed(float acceleration, float target_velocity, float distance)
{
    // to avoid invalid negative numbers due to numerical errors
    float value = std::max(0.0f, sqr(target_velocity) - 2.0f * acceleration * distance);
    return std::sqrt(value);
}

static inline float acceleration_time_from_distance(float initial_feedrate, float distance, float acceleration)
{
    return (acceleration != 0.0f) ? (speed_from_distance(initial_feedrate, distance, acceleration) - initial_feedrate) / acceleration : 0.0f;
}

struct PrintEstimatedStatistics
{
    enum class ETimeMode : unsigned char
    {
        Normal,
        Stealth,
        Count
    };

    struct Mode
    {
        float time;
        float prepare_time;
        std::vector<std::pair<CustomGCode::Type, std::pair<float, float>>> custom_gcode_times;
        std::vector<std::pair<EMoveType, float>> moves_times;
        std::vector<std::pair<ExtrusionRole, float>> roles_times;
        std::vector<float> layers_times;

        void reset()
        {
            time = 0.0f;
            prepare_time = 0.0f;
            custom_gcode_times.clear();
            custom_gcode_times.shrink_to_fit();
            moves_times.clear();
            moves_times.shrink_to_fit();
            roles_times.clear();
            roles_times.shrink_to_fit();
            layers_times.clear();
            layers_times.shrink_to_fit();
        }
    };

    std::vector<double> volumes_per_color_change;
    std::map<size_t, double> volumes_per_extruder;
    // BBS: the flush amount of every filament
    std::map<size_t, double> flush_per_filament;
    std::map<ExtrusionRole, std::pair<double, double>> used_filaments_per_role;

    std::array<Mode, static_cast<size_t>(ETimeMode::Count)> modes;
    unsigned int total_filamentchanges;

    PrintEstimatedStatistics() { reset(); }

    void reset()
    {
        for (auto m : modes)
        {
            m.reset();
        }
        volumes_per_color_change.clear();
        volumes_per_color_change.shrink_to_fit();
        volumes_per_extruder.clear();
        flush_per_filament.clear();
        used_filaments_per_role.clear();
        total_filamentchanges = 0;
    }
};

struct MoveVertex
{
    unsigned int gcode_id{0};
    EMoveType type{EMoveType::Noop};
    ExtrusionRole extrusion_role{erNone};
    unsigned char extruder_id{0};
    unsigned char cp_color_id{0};
    Vec3f position{Vec3f()};    // mm
    float delta_extruder{0.0f}; // mm
    float feedrate{0.0f};       // mm/s
    float width{0.0f};          // mm
    float height{0.0f};         // mm
    float mm3_per_mm{0.0f};
    float fan_speed{0.0f};      // percentage
    float temperature{0.0f};    // Celsius degrees
    float time{0.0f};           // s
    float layer_duration{0.0f}; // s (layer id before finalize)

    // BBS: arc move related data
    EMovePathType move_path_type{EMovePathType::Noop_move};
    Vec3f arc_center_position{Vec3f()};      // mm
    std::vector<Vec3f> interpolation_points; // interpolation points of arc for drawing

    float volumetric_rate() const { return feedrate * mm3_per_mm; }
    // BBS: new function to support arc move
    bool is_arc_move_with_interpolation_points() const
    {
        return (move_path_type == EMovePathType::Arc_move_ccw || move_path_type == EMovePathType::Arc_move_cw) && interpolation_points.size();
    }
    bool is_arc_move() const
    {
        return move_path_type == EMovePathType::Arc_move_ccw || move_path_type == EMovePathType::Arc_move_cw;
    }
};

struct CachedPosition
{
    AxisCoords position; // mm
    float feedrate;      // mm/s

    void reset();
};

struct CpColor
{
    unsigned char counter;
    unsigned char current;

    void reset();
};

struct UsedFilaments // filaments per ColorChange
{
    double color_change_cache;
    std::vector<double> volumes_per_color_change;

    double tool_change_cache;
    std::map<size_t, double> volumes_per_extruder;

    // BBS: the flush amount of every filament
    std::map<size_t, double> flush_per_filament;

    double role_cache;
    std::map<ExtrusionRole, std::pair<double, double>> filaments_per_role;

    void reset();

    void increase_caches(double extruded_volume);

    void process_color_change_cache();
    void process_extruder_cache(GCodeProcessor *processor);
    void update_flush_per_filament(size_t extrude_id, float flush_length);
    void process_role_cache(GCodeProcessor *processor);
    void process_caches(GCodeProcessor *processor);

    friend class GCodeProcessor;
};

// TimeProcessor Ïà¹Ø

// static inline void set_option_value(ConfigOptionFloats& option, size_t id, float value)
//{
//     if (id < option.values.size())
//         option.values[id] = static_cast<double>(value);
// };
//
// static inline float get_option_value(const ConfigOptionFloats& option, size_t id)
//{
//     return option.values.empty() ? 0.0f :
//         ((id < option.values.size()) ? static_cast<float>(option.values[id]) : static_cast<float>(option.values.back()));
// }

// struct TimeBlock
//{
//     struct Flags
//     {
//         bool recalculate{ false };
//         bool nominal_length{ false };
//         bool prepare_stage{ false };
//     };
//
//     struct FeedrateProfile
//     {
//         float entry{ 0.0f }; // mm/s
//         float cruise{ 0.0f }; // mm/s
//         float exit{ 0.0f }; // mm/s
//     };
//
//     struct Trapezoid
//     {
//         float accelerate_until{ 0.0f }; // mm
//         float decelerate_after{ 0.0f }; // mm
//         float cruise_feedrate{ 0.0f }; // mm/sec
//
//         float acceleration_time(float entry_feedrate, float acceleration) const;
//         float cruise_time() const;
//         float deceleration_time(float distance, float acceleration) const;
//         float cruise_distance() const;
//     };
//
//     EMoveType move_type{ EMoveType::Noop };
//     ExtrusionRole role{ erNone };
//     unsigned int g1_line_id{ 0 };
//     unsigned int layer_id{ 0 };
//     float distance{ 0.0f }; // mm
//     float acceleration{ 0.0f }; // mm/s^2
//     float max_entry_speed{ 0.0f }; // mm/s
//     float safe_feedrate{ 0.0f }; // mm/s
//     Flags flags;
//     FeedrateProfile feedrate_profile;
//     Trapezoid trapezoid;
//
//     // Calculates this block's trapezoid
//     void calculate_trapezoid();
//
//     float time() const;
// };

// struct TimeMachine
//{
//     struct State
//     {
//         float feedrate; // mm/s
//         float safe_feedrate; // mm/s
//         //BBS: feedrate of X-Y-Z-E axis. But when the move is G2 and G3, X-Y will be
//         //same value which means feedrate in X-Y plane.
//         AxisCoords axis_feedrate; // mm/s
//         AxisCoords abs_axis_feedrate; // mm/s
//
//         //BBS: unit vector of enter speed and exit speed in x-y-z space.
//         //For line move, there are same. For arc move, there are different.
//         Vec3f enter_direction;
//         Vec3f exit_direction;
//
//         void reset();
//     };
//
//     struct CustomGCodeTime
//     {
//         bool needed;
//         float cache;
//         std::vector<std::pair<CustomGCode::Type, float>> times;
//
//         void reset();
//     };
//
//     struct G1LinesCacheItem
//     {
//         unsigned int id;
//         float elapsed_time;
//     };
//
//     bool enabled;
//     float acceleration; // mm/s^2
//     // hard limit for the acceleration, to which the firmware will clamp.
//     float max_acceleration; // mm/s^2
//     float retract_acceleration; // mm/s^2
//     // hard limit for the acceleration, to which the firmware will clamp.
//     float max_retract_acceleration; // mm/s^2
//     float travel_acceleration; // mm/s^2
//     // hard limit for the travel acceleration, to which the firmware will clamp.
//     float max_travel_acceleration; // mm/s^2
//     float extrude_factor_override_percentage;
//     float time; // s
//     struct StopTime
//     {
//         unsigned int g1_line_id;
//         float elapsed_time;
//     };
//     std::vector<StopTime> stop_times;
//     std::string line_m73_main_mask;
//     std::string line_m73_stop_mask;
//     State curr;
//     State prev;
//     CustomGCodeTime gcode_time;
//     std::vector<TimeBlock> blocks;
//     std::vector<G1LinesCacheItem> g1_times_cache;
//     std::array<float, static_cast<size_t>(EMoveType::Count)> moves_time;
//     std::array<float, static_cast<size_t>(ExtrusionRole::erCount)> roles_time;
//     std::vector<float> layers_time;
//     //BBS: prepare stage time before print model, including start gcode time and mostly same with start gcode time
//     float prepare_time;
//
//     void reset();
//
//     // Simulates firmware st_synchronize() call
//     void simulate_st_synchronize(float additional_time = 0.0f);
//     void calculate_time(size_t keep_last_n_blocks = 0, float additional_time = 0.0f);
// };

// struct TimeProcessor
//{
//     struct Planner
//     {
//         // Size of the firmware planner queue. The old 8-bit Marlins usually just managed 16 trapezoidal blocks.
//         // Let's be conservative and plan for newer boards with more memory.
//         static constexpr size_t queue_size = 64;
//         // The firmware recalculates last planner_queue_size trapezoidal blocks each time a new block is added.
//         // We are not simulating the firmware exactly, we calculate a sequence of blocks once a reasonable number of blocks accumulate.
//         static constexpr size_t refresh_threshold = queue_size * 4;
//     };
//
//     // extruder_id is currently used to correctly calculate filament load / unload times into the total print time.
//     // This is currently only really used by the MK3 MMU2:
//     // extruder_unloaded = true means no filament is loaded yet, all the filaments are parked in the MK3 MMU2 unit.
//     bool extruder_unloaded;
//     // allow to skip the lines M201/M203/M204/M205 generated by GCode::print_machine_envelope() for non-Normal time estimate mode
//     bool machine_envelope_processing_enabled;
//     MachineEnvelopeConfig machine_limits;
//     // Additional load / unload times for a filament exchange sequence.
//     float filament_load_times;
//     float filament_unload_times;
//     std::array<TimeMachine, static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Count)> machines;
//
//     void reset();
//
//     // post process the file with the given filename to add remaining time lines M73
//     // and updates moves' gcode ids accordingly
//     void post_process(const std::string& filename, std::vector<MoveVertex>& moves, std::vector<size_t>& lines_ends, size_t total_layer_num);
// };

class SeamsDetector
{
    bool m_active{false};
    std::optional<Vec3f> m_first_vertex;

public:
    void activate(bool active)
    {
        if (m_active != active)
        {
            m_active = active;
            if (m_active)
                m_first_vertex.reset();
        }
    }

    std::optional<Vec3f> get_first_vertex() const { return m_first_vertex; }
    void set_first_vertex(const Vec3f &vertex) { m_first_vertex = vertex; }

    bool is_active() const { return m_active; }
    bool has_first_vertex() const { return m_first_vertex.has_value(); }
};

// class Circle {
// public:
//     Circle() {
//         center = Point(0, 0);
//         radius = 0;
//     }
//     Circle(Point& p, double r) {
//         center = p;
//         radius = r;
//     }
//     Point center;
//     double radius;
//
//     Point get_closest_point(const Point& input) {
//         Vec2d v = (input - center).cast<double>().normalized();
//         return (center + (v * radius).cast<coord_t>());
//     }
//
//     static bool try_create_circle(const Point& p1, const Point& p2, const Point& p3, const double max_radius, Circle& new_circle);
//     static bool try_create_circle(const Points& points, const double max_radius, const double tolerance, Circle& new_circle);
//     double get_polar_radians(const Point& p1) const;
//     bool is_over_deviation(const Points& points, const double tolerance);
//     bool get_deviation_sum_squared(const Points& points, const double tolerance, double& sum_deviation);
//
//     //BBS: only support calculate on X-Y plane, Z is useless
//     static Vec3f calc_tangential_vector(const Vec3f& pos, const Vec3f& center_pos, const bool is_ccw);
//     static bool get_closest_perpendicular_point(const Point& p1, const Point& p2, const Point& c, Point& out);
//     static bool is_equal(double x, double y, double tolerance = ZERO_TOLERANCE) {
//         double abs_difference = std::fabs(x - y);
//         return abs_difference < tolerance;
//     };
//     static bool greater_than(double x, double y, double tolerance = ZERO_TOLERANCE) {
//         return x > y && !Circle::is_equal(x, y, tolerance);
//     };
//     static bool greater_than_or_equal(double x, double y, double tolerance = ZERO_TOLERANCE) {
//         return x > y || Circle::is_equal(x, y, tolerance);
//     };
//     static bool less_than(double x, double y, double tolerance = ZERO_TOLERANCE) {
//         return x < y && !Circle::is_equal(x, y, tolerance);
//     };
//     static bool less_than_or_equal(double x, double y, double tolerance = ZERO_TOLERANCE) {
//         return x < y || Circle::is_equal(x, y, tolerance);
//     };
// };
//
// class ArcSegment : public Circle {
// public:
//     ArcSegment() : Circle() {}
//     ArcSegment(Point center, double radius, Point start, Point end, ArcDirection dir) :
//         Circle(center, radius),
//         start_point(start),
//         end_point(end),
//         direction(dir) {
//         if (radius == 0.0 ||
//             start_point == center ||
//             end_point == center ||
//             start_point == end_point) {
//             is_arc = false;
//             return;
//         }
//         update_angle_and_length();
//         is_arc = true;
//     }
//
//     bool is_arc = false;
//     double length = 0;
//     double angle_radians = 0;
//     double polar_start_theta = 0;
//     double polar_end_theta = 0;
//     Point start_point{ Point(0,0) };
//     Point end_point{ Point(0,0) };
//     ArcDirection direction = ArcDirection::Arc_Dir_unknow;
//
//     bool is_valid() const { return is_arc; }
//     bool clip_start(const Point& point);
//     bool clip_end(const Point& point);
//     bool reverse();
//     bool split_at(const Point& point, ArcSegment& p1, ArcSegment& p2);
//     bool is_point_inside(const Point& point) const;
//
// private:
//     void update_angle_and_length();
//
// public:
//     static bool try_create_arc(
//         const Points& points,
//         ArcSegment& target_arc,
//         double approximate_length,
//         double max_radius = DEFAULT_SCALED_MAX_RADIUS,
//         double tolerance = DEFAULT_SCALED_RESOLUTION,
//         double path_tolerance_percent = DEFAULT_ARC_LENGTH_PERCENT_TOLERANCE);
//
//     static bool are_points_within_slice(const ArcSegment& test_arc, const Points& points);
//     // BBS: this function is used to detect whether a ray cross the segment
//     static bool ray_intersects_segment(const Point& rayOrigin, const Vec2d& rayDirection, const Line& segment);
//     // BBS: these three functions are used to calculate related arguments of arc in unscale_field.
//     static float calc_arc_radian(Vec3f start_pos, Vec3f end_pos, Vec3f center_pos, bool is_ccw);
//     static float calc_arc_radius(Vec3f start_pos, Vec3f center_pos);
//     static float calc_arc_length(Vec3f start_pos, Vec3f end_pos, Vec3f center_pos, bool is_ccw);
// private:
//     static bool try_create_arc(
//         const Circle& c,
//         const Point& start_point,
//         const Point& mid_point,
//         const Point& end_point,
//         ArcSegment& target_arc,
//         double approximate_length,
//         double path_tolerance_percent = DEFAULT_ARC_LENGTH_PERCENT_TOLERANCE);
// };

#endif // !GcodeData_hpp
