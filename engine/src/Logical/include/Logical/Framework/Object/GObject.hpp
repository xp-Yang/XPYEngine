#ifndef GObject_hpp
#define GObject_hpp

#include "Base/Signal/Signal.hpp"
#include "Logical/Framework/World/SceneDirty.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"
#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/AnimationComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Component/RigidComponent.hpp"

#include <type_traits>

namespace Meta {
	namespace Registration {
		void allMetaRegister();
	}
}

class GObjectID : public IDAllocator<GObjectID> {
public:
	GObjectID() = default;
};

class GObject {
public:
	static GObject* create(GObject* parent, const std::string& name);
	~GObject();
	void append(GObject* node) { m_children.push_back(node); }
	int index() const { return m_parent ? m_parent->indexOf(this) : -1; }
	int indexOf(const GObject* child) const;
	bool remove(GObject* node = nullptr);
	bool include(const GObject* node);
	const std::vector<GObject*> allLeaves2();
	const std::vector<GObject*> allLeaves();
	bool isLeaf() const { return m_children.empty(); }
	const std::vector<GObject*>& children() const { return m_children; }

	bool load();
	void save();

	GObjectID ID() const { return m_id; }
	const std::string& name() const { return m_name; }
	void setName(const std::string& name) { m_name = name; }
	bool visible() const { return m_visible; }
	void setVisible(bool visible) {
		if (m_visible == visible)
			return;
		m_visible = visible;
		markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Visibility));
	}

	void markDirty(SceneDirtyFlags flags) {
		if (flags != SceneDirtyFlagBit(SceneDirtyFlag::None))
			emit dirty(m_id, flags);
	}

	const std::vector<std::shared_ptr<Component>>& getComponents() const { return m_components; }

	template<typename T>
	T& addComponent() {
		auto com = std::make_shared<T>(this);
		m_components.push_back(com);
		markDirty(dirtyFlagsForComponent<T>());
		return *com;
	}

	template<typename T>
	bool hasComponent() const {
		for (auto& component : m_components)
		{
			if (typeid(T) == typeid(*component))
				return true;
		}
		return false;
	}

	template<typename T>
	T* getComponent()
	{
		for (auto& component : m_components)
		{
			if (typeid(T) == typeid(*component))
				return static_cast<T*>(component.get());
		}
		return nullptr;
	}

	virtual void tick(float delta_time);

signals:
	Signal<GObjectID, SceneDirtyFlags> dirty;

protected:
	GObject(GObject* parent, const std::string& name) : m_parent(parent), m_name(name) { if (parent) parent->append(this); }
	GObject(const GObject&) = delete;
	GObject& operator=(const GObject&) = delete;

	friend void Meta::Registration::allMetaRegister();

	template<typename T>
	SceneDirtyFlags dirtyFlagsForComponent() const {
		if constexpr (std::is_same_v<T, TransformComponent>)
			return SceneDirtyFlagBit(SceneDirtyFlag::Transform);
		else if constexpr (std::is_same_v<T, MeshComponent>)
			return SceneDirtyFlagBit(SceneDirtyFlag::Mesh);
		else if constexpr (std::is_base_of_v<LightComponent, T>)
			return SceneDirtyFlagBit(SceneDirtyFlag::Light);
		else if constexpr (std::is_same_v<T, CameraComponent>)
			return SceneDirtyFlagBit(SceneDirtyFlag::Camera);
		else if constexpr (std::is_same_v<T, AnimationComponent>)
			return SceneDirtyFlagBit(SceneDirtyFlag::Mesh);
		else
			return SceneDirtyFlagBit(SceneDirtyFlag::None);
	}

	GObject* m_parent;
	std::vector<GObject*> m_children;
	GObjectID m_id;
	std::string m_name;
	bool m_visible{ true };
	std::vector<std::shared_ptr<Component>> m_components;
};

#endif
