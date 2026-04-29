#include "Logical/Framework/Object/GObject.hpp"

GObject *GObject::create(GObject *parent, const std::string &name)
{
	GObject *obj = new GObject(parent, name);
	return obj;
}

GObject::~GObject()
{
	for (auto child : m_children)
	{
		delete child;
	}
}

int GObject::indexOf(const GObject *child) const
{
	if (!child)
		return -1;
	for (int i = 0; i < m_children.size(); i++)
	{
		if (m_children[i]->m_id == child->m_id)
		{
			return i;
		}
	}
	return -1;
}

bool GObject::remove(GObject *node)
{
	if (node == nullptr)
	{
		node = this;
	}
	if (include(node))
	{
		if (node->m_parent)
		{
			node->m_parent->m_children.erase(node->m_parent->m_children.begin() + node->index());
		}
		delete node;
		return true;
	}
	return false;
}

bool GObject::include(const GObject *node)
{
	if (node == this)
		return true;
	for (auto child : m_children)
	{
		if (child->include(node))
			return true;
	}
	return false;
}

const std::vector<GObject *> GObject::allLeaves2()
{
	if (isLeaf())
		return {this};
	// depth-first
	std::vector<GObject *> leaves;
	for (auto child : m_children)
	{
		auto child_leaves = child->allLeaves2();
		leaves.insert(leaves.end(), child_leaves.begin(), child_leaves.end());
	}
	return leaves;
}

const std::vector<GObject *> GObject::allLeaves()
{
	if (isLeaf())
		return {this};
	// breadth-first
	std::vector<GObject *> leaves;
	std::vector<GObject *> nodes;
	nodes.push_back(this);
	while (!nodes.empty())
	{
		auto node = nodes.front();
		nodes.erase(nodes.begin());
		for (auto child : node->children())
		{
			if (!child->children().empty())
			{
				nodes.push_back(child);
			}
			else
			{
				leaves.push_back(child);
			}
		}
	}
	return leaves;
}

bool GObject::load()
{
	return false;
}

void GObject::save()
{
}

void GObject::tick(float delta_time)
{
	// auto& world = World::get();
	//// light
	// float avrg_frame_time = 1.0f / ImGui::GetIO().Framerate;
	//// 每一帧持续时间越长，意味着上一帧的渲染花费了越多时间，所以这一帧的速度应该越大，来平衡渲染所花去的时间
	// float delta = avrg_frame_time / 2.0f;
	// Mat4 rotate = Rotate(delta, Vec3(0.0f, 1.0f, 0.0f));
	// for (auto entity : world.entityView<PointLightComponent>()) {
	//	auto& light_transform = *world.getComponent<TransformComponent>(entity);
	//	light_transform.translation = Vec3(rotate * Vec4(light_transform.translation, 1.0f));
	// }
	////auto dir_light_component = world.getMainDirectionalLightComponent();
	////dir_light_component->direction = Vec3(rotate * Vec4(dir_light_component->direction, 1.0f));
}
