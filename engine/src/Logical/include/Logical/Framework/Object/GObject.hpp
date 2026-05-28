#ifndef GObject_hpp
#define GObject_hpp

#include "Base/Signal/Signal.hpp"
#include "Logical/Framework/World/RenderDirty.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"
#if !ENABLE_ECS
#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/AnimationComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Component/RigidComponent.hpp"
#else
#include "Logical/Framework/ECS/Components.hpp"
#endif

#include <type_traits>

namespace Meta {
	namespace Registration {
		void allMetaRegister();
	}
}

#if !ENABLE_ECS
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
		markRenderDirty(RenderDirtyFlagBit(RenderDirtyFlag::Visibility));
	}

	void markRenderDirty(RenderDirtyFlags flags) {
		if (flags != RenderDirtyFlagBit(RenderDirtyFlag::None))
			emit renderDirty(m_id, flags);
	}

	// TODO: Third round: wrap TransformComponent / LightComponent / CameraComponent
	// writes in setters or markDirty() helpers so editor and runtime code do not
	// manually emit render dirty flags after mutating component fields.

	const std::vector<std::shared_ptr<Component>>& getComponents() const { return m_components; }

	template<typename T>
	T& addComponent() {
		auto com = std::make_shared<T>(this);
		m_components.push_back(com);
		markRenderDirty(renderDirtyFlagsForComponent<T>());
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
	Signal<GObjectID, RenderDirtyFlags> renderDirty;

protected:
	GObject(GObject* parent, const std::string& name) : m_parent(parent), m_name(name) { if (parent) parent->append(this); }
	GObject(const GObject&) = delete;
	GObject& operator=(const GObject&) = delete;

	friend void Meta::Registration::allMetaRegister();

	template<typename T>
	RenderDirtyFlags renderDirtyFlagsForComponent() const {
		if constexpr (std::is_same_v<T, TransformComponent>)
			return RenderDirtyFlagBit(RenderDirtyFlag::Transform);
		else if constexpr (std::is_same_v<T, MeshComponent>)
			return RenderDirtyFlagBit(RenderDirtyFlag::Mesh);
		else if constexpr (std::is_base_of_v<LightComponent, T>)
			return RenderDirtyFlagBit(RenderDirtyFlag::Light);
		else if constexpr (std::is_same_v<T, CameraComponent>)
			return RenderDirtyFlagBit(RenderDirtyFlag::Camera);
		else if constexpr (std::is_same_v<T, AnimationComponent>)
			return RenderDirtyFlagBit(RenderDirtyFlag::Mesh);
		else
			return RenderDirtyFlagBit(RenderDirtyFlag::None);
	}

	GObject* m_parent;
	std::vector<GObject*> m_children;
	GObjectID m_id;
	std::string m_name;
	bool m_visible{ true };
	std::vector<std::shared_ptr<Component>> m_components;
};

#else
class GObject {
public:
	GObject(const ecs::Entity& entity) : GObject(nullptr, entity) {}
	GObject(GObject* parent, const ecs::Entity& entity) : m_parent(parent), m_entity(entity) { if (parent) parent->append(this); }
	void append(GObject* node) { m_children.push_back(node); }
	int index() const {
		return m_parent ? m_parent->indexOf(this) : -1;
	}
	int indexOf(const GObject* child) const {
		if (!child)
			return -1;
		for (int i = 0; i < m_children.size(); i++) {
			if (m_children[i]->entity() == child->entity()) {
				return i;
			}
		}
		return -1;
	}
	void remove(const ecs::Entity& entity) {
		for (auto child : m_children) {
			child->remove(entity);
		}
		auto it = std::find_if(m_children.begin(), m_children.end(), [entity](GObject* child) {
			return (*child).entity().getId() == entity.getId();
			});
		if (it != m_children.end()) {
			ecs::World::get().removeComponent<AllComponents>(entity);
			m_children.erase(it);
		}
	}
	void remove(GObject* node) { remove(node->entity()); }
	GObject* find(const ecs::Entity& entity) {
		for (auto child : m_children) {
			auto res = child->find(entity);
			if (res)
				return res;
		}
		return m_entity.getId() == entity.getId() ? this : nullptr;
	}
	const std::vector<GObject*> allLeaves2() {
		if (isLeaf())
			return { this };
		// 深度优先
		std::vector<GObject*> leaves;
		for (auto child : m_children) {
			if (child->children().empty()) {
				leaves.push_back(child);
			}
			else {
				auto child_leaves = child->allLeaves2();
				leaves.insert(leaves.end(), child_leaves.begin(), child_leaves.end());
			}
		}
		return leaves;
	}
	const std::vector<GObject*> allLeaves() {
		if (isLeaf())
			return { this };
		// 广度优先
		std::vector<GObject*> leaves;
		std::vector<GObject*> nodes;
		nodes.push_back(this);
		while (!nodes.empty()) {
			auto node = nodes.front();
			nodes.erase(nodes.begin());
			for (auto child : node->children()) {
				if (!child->children().empty()) {
					nodes.push_back(child);
				}
				else {
					leaves.push_back(child);
				}
			}
		}
		return leaves;
	}
	bool isLeaf() const { return m_children.empty(); }
	const std::vector<GObject*>& children() const { return m_children; }
	const ecs::Entity& entity() const { return m_entity; }

private:
	GObject* m_parent;
	std::vector<GObject*> m_children;
	const ecs::Entity m_entity;
};
#endif // !

#endif
