#ifndef MaterialBallTest_hpp
#define MaterialBallTest_hpp

#include <map>
#include <memory>
#include <string>
#include <vector>

class Scene;
struct Material;

// Shader Ball Gallery：以三角阵列陈列一组 PBR 材质球，作为
// IBL / Tone Mapping / 法线贴图 / PCF 阴影的回归对照场景。
// 模型与贴图按 engine/asset/material_ball/ 约定加载，缺失时回退到
// 内置球体与预设材质，保证无资产时也能直接运行。
class MaterialBallTest
{
public:
    void init();

private:
    struct GalleryLayout
    {
        float                    spacing{ 3.0f };
        float                    lift{ 1.0f };   // 球心抬高，使单位球落在地面上
        float                    scale{ 0.01f }; // shader_ball 模型尺度，写入 TransformComponent::scale
        float                    origin[3]{ 0.0f, 0.0f, 0.0f };
        std::vector<int>         row_counts;
        std::vector<std::string> materials;
    };

    std::string loadShaderBallMesh() const;
    std::map<std::string, std::shared_ptr<Material>> loadMaterialLibrary() const;
    GalleryLayout loadLayout(const std::vector<std::string>& available) const;

    void spawnGallery(Scene* scene);
    void createGround(Scene* scene);
    void createLighting(Scene* scene);

    std::string m_root_dir;  // engine/asset/material_ball
    std::string m_mesh_path; // 已解析的 shader ball 网格路径
};

#endif
