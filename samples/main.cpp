#include "Engine.hpp"
#include "Base/Utils/PathService.hpp"
#include "Cube/cubetest.hpp"
#include "GcodePreview/GcodePreview.hpp"

int main()
{
    auto& engine = Engine::get();

    auto gcode_feature = std::make_shared<GcodePreview>();
    // Sample layer intercepts .gcode and owns related business logic.
    engine.setExternalOpenFileHandler([&engine, gcode_feature](const std::string& filepath) {
        if (PathService::getFileSuffix(filepath) != "gcode")
            return false;
        return gcode_feature->loadFromFile(filepath, engine.Scene());
    });
    engine.setExternalGuiCallback([gcode_feature]() { gcode_feature->renderGui(); });
    engine.init();

    Cubetest* cubetest = new Cubetest();
    cubetest->init();

    engine.run();
    engine.shutdown();
    return 0;
}

int WinMain()
{
    return main();
}
