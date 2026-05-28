#ifndef MaterialBallTest_hpp
#define MaterialBallTest_hpp

class Scene;

class MaterialBallTest {
public:
    void init();

private:
    void createMaterialBallGrid(Scene* scene);
    void createShadowTestObjects(Scene* scene);
    void createLighting(Scene* scene);
    void createNanosuitModel(Scene* scene);
};

#endif
