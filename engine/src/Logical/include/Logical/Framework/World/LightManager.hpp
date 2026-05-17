#ifndef LightManager_hpp
#define LightManager_hpp

#include "Base/Math/Math.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"

class LightID : public IDAllocator<LightID>
{
public:
	LightID() = default;
};

class Light
{
public:
	Light(const std::string &name) : m_name(name) {}
    Color3 luminousColor = { 1.0f, 1.0f, 1.0f };
	LightID ID() const { return m_id; }
	std::string name() const { return m_name; }
	virtual Mat4 lightProjMatrix() const = 0;

protected:
	LightID m_id;
	std::string m_name;
};

class PointLight : public Light
{
public:
	PointLight() : Light("Point Light") {}

	float radius;
	Vec3 position;

	Mat4 lightProjMatrix() const override
	{
		Mat4 light_projection = Math::Perspective(Math::deg2rad(90.0f), 1.0f, 0.1f, radius);
		return light_projection;
	}

	std::array<Mat4, 6> lightViewMatrix() const
	{
		Vec3 light_pos = position;

		// 这里up向量朝下，因为cubeMap从内部采样，是反过来的
		// TODO 点阴影贴图up矢量朝下，那么上下不也颠倒了
		Mat4 light_view_right = Math::LookAt(light_pos, light_pos + Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)); // 右
		Mat4 light_view_left = Math::LookAt(light_pos, light_pos + Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)); // 左
		Mat4 light_view_up = Math::LookAt(light_pos, light_pos + Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));	  // 上
		Mat4 light_view_down = Math::LookAt(light_pos, light_pos + Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f)); // 下
		Mat4 light_view_front = Math::LookAt(light_pos, light_pos + Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f)); // 近
		Mat4 light_view_back = Math::LookAt(light_pos, light_pos + Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f)); // 远

		std::array<Mat4, 6> result;
		result[0] = light_view_right;
		result[1] = light_view_left;
		result[2] = light_view_up;
		result[3] = light_view_down;
		result[4] = light_view_front;
		result[5] = light_view_back;
		return result;
	}
};

class DirectionalLight : public Light
{
public:
	DirectionalLight() : Light("Directional Light") {}

	Vec3 direction;
	// TODO 
	// 问题：阴影覆盖过窄/过宽、边缘裁剪、分辨率利用率变差。
	// 解决方案： “按相机可见体拟合”的 CSM/动态阴影视锥
	float aspectRatio{16.0f / 9.0f}; 

	Mat4 lightProjMatrix() const override
	{
		Mat4 light_projection = Math::Ortho(-30.0f * aspectRatio, 30.0f * aspectRatio, -30.0f, 30.0f, 0.1f, 100.0f);
		return light_projection;
	}

	Mat4 lightViewMatrix() const
	{
		Mat4 light_view = Math::LookAt(Vec3(0.0f) - direction, Vec3(0.0f), Vec3(0.0f, 1.0f, 0.0f));
		return light_view;
	}
};

class LightManager
{
public:
	LightManager();
	std::shared_ptr<DirectionalLight> mainDirectionalLight() const
	{
		return std::dynamic_pointer_cast<DirectionalLight>(m_lights[0]);
	}
	std::vector<std::shared_ptr<PointLight>> pointLights() const
	{
		std::vector<std::shared_ptr<PointLight>> res;
		for (const auto &light : m_lights)
		{
			if (light != mainDirectionalLight())
				res.push_back(std::dynamic_pointer_cast<PointLight>(light));
		}
		return res;
	}
	const std::vector<std::shared_ptr<Light>> &lights() const { return m_lights; }

	void addPointLight();
	void removeLastPointLight();
	void addDirectionalLight();

private:
	std::vector<std::shared_ptr<Light>> m_lights;
};

#endif // !LightManager_hpp
