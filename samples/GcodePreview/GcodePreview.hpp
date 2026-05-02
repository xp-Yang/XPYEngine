#ifndef GcodePreview_HPP
#define GcodePreview_HPP

#include "Logical/Framework/World/Scene.hpp"
#include "GcodeImporter/GcodeResultData.hpp"
#include "GcodeTrace.hpp"

class GcodePreview {
public:
    GcodePreview();
    bool loadFromFile(const std::string& filepath, const std::shared_ptr<Scene>& scene);
    void renderGui();

protected:
    void buildMesh(const std::array<LinesBatch, ExtrusionRole::erCount>& lines_batches);
    void rebuildMeshRange(const std::array<LinesBatch, ExtrusionRole::erCount>& lines_batches);

private:
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<GObject> m_gcodes_object;
    std::unique_ptr<GcodeTrace> m_gcode_trace;

    GCodeProcessorResult m_result;
    std::string m_source_filepath;
    bool m_loaded{ false };
};

#endif // SAMPLE_GCODE_FEATURE_HPP
