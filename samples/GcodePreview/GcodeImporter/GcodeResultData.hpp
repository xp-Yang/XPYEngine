#ifndef GcodeResultData_hpp
#define GcodeResultData_hpp

#include "Base/Common.hpp"
#include "GcodeData.hpp"

struct GCodeProcessorResult
{
    struct SettingsIds
    {
        std::string print;
        std::vector<std::string> filament;
        std::string printer;

        void reset() {
            print.clear();
            filament.clear();
            printer.clear();
        }
    };

    struct SliceWarning {
        int         level;                  // 0: normal tips, 1: warning; 2: error
        std::string msg;                    // enum string
        std::string error_code;             // error code for studio
        std::vector<std::string> params;    // extra msg info
    };

    std::string filename;
    unsigned int id;
    std::vector<MoveVertex> moves;
    // Positions of ends of lines of the final G-code this->filename after TimeProcessor::post_process() finalizes the G-code.
    std::vector<size_t> lines_ends;
    Pointfs printable_area;
    //BBS: add bed exclude area
    Pointfs bed_exclude_area;
    //BBS: add toolpath_outside
    bool toolpath_outside;
    //BBS: add object_label_enabled
    bool label_object_enabled;
    float printable_height;
    SettingsIds settings_ids;
    size_t extruders_count;
    std::vector<std::string> extruder_colors;
    std::vector<float> filament_diameters;
    std::vector<int>   required_nozzle_HRC;
    std::vector<float> filament_densities;
    std::vector<int> filament_vitrification_temperature;
    PrintEstimatedStatistics print_statistics;
    std::vector<CustomGCode::Item> custom_gcode_per_print_z;
    std::vector<std::pair<float, std::pair<size_t, size_t>>> spiral_vase_layers;
    std::vector<SliceWarning> warnings;
    int nozzle_hrc;
    BedType bed_type = BedType::btCount;

    void reset();

    ////BBS: add mutex for protection of gcode result
    //mutable std::mutex result_mutex;
    //void  lock() const { result_mutex.lock(); }
    //void  unlock() const { result_mutex.unlock(); }
};

// Helper class used to fix the z for color change, pause print and
// custom gcode markes
class OptionsZCorrector
{
    GCodeProcessorResult& m_result;
    std::optional<size_t> m_move_id;
    std::optional<size_t> m_custom_gcode_per_print_z_id;

public:
    explicit OptionsZCorrector(GCodeProcessorResult& result) : m_result(result) {}

    void set() {
        m_move_id = m_result.moves.size() - 1;
        m_custom_gcode_per_print_z_id = m_result.custom_gcode_per_print_z.size() - 1;
    }

    void update(float height) {
        if (!m_move_id.has_value() || !m_custom_gcode_per_print_z_id.has_value())
            return;

        const Vec3f position = m_result.moves.back().position;

        MoveVertex& move = m_result.moves.emplace_back(m_result.moves[*m_move_id]);
        move.position = position;
        move.height = height;
        m_result.moves.erase(m_result.moves.begin() + *m_move_id);
        m_result.custom_gcode_per_print_z[*m_custom_gcode_per_print_z_id].print_z = position.z;
        reset();
    }

    void reset() {
        m_move_id.reset();
        m_custom_gcode_per_print_z_id.reset();
    }
};

#endif // !GcodeResult_hpp
