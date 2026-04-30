#ifndef AnimationComponent_hpp
#define AnimationComponent_hpp

#include "Logical/Framework/Component/Component.hpp"
#include <memory>
#include <string>

class Animation;

struct AnimationComponent : public Component {
	AnimationComponent(GObject* parent) : Component(parent) {}
	std::shared_ptr<Animation> clip{ nullptr };
	std::string clip_path;
	float speed{ 1.0f };
	bool loop{ true };
	bool playing{ true };

};

#endif // !AnimationComponent_hpp
