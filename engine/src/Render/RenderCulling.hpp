#ifndef RenderCulling_hpp
#define RenderCulling_hpp

#include "Base/Common.hpp"

#include <array>

struct RenderAABB {
    Vec3 min{ 0.0f };
    Vec3 max{ 0.0f };
    bool valid{ false };
};

struct RenderFrustumPlane {
    Vec3 normal{ 0.0f };
    float distance{ 0.0f };
};

struct RenderFrustum {
    static RenderFrustum fromViewProjection(const Mat4& view_projection)
    {
        const Vec4 row0 = rowOf(view_projection, 0);
        const Vec4 row1 = rowOf(view_projection, 1);
        const Vec4 row2 = rowOf(view_projection, 2);
        const Vec4 row3 = rowOf(view_projection, 3);

        return RenderFrustum{
            {
                planeOf(row3 + row0),
                planeOf(row3 - row0),
                planeOf(row3 + row1),
                planeOf(row3 - row1),
                planeOf(row3 + row2),
                planeOf(row3 - row2),
            }
        };
    }

    bool intersects(const RenderAABB& bounds) const
    {
        if (!bounds.valid)
            return true;

        for (const RenderFrustumPlane& plane : planes)
        {
            Vec3 positive = bounds.min;
            if (plane.normal.x >= 0.0f)
                positive.x = bounds.max.x;
            if (plane.normal.y >= 0.0f)
                positive.y = bounds.max.y;
            if (plane.normal.z >= 0.0f)
                positive.z = bounds.max.z;

            if (Math::Dot(plane.normal, positive) + plane.distance < 0.0f)
                return false;
        }
        return true;
    }

    std::array<RenderFrustumPlane, 6> planes;

private:
    static Vec4 rowOf(const Mat4& matrix, int row)
    {
        return Vec4(matrix[0][row], matrix[1][row], matrix[2][row], matrix[3][row]);
    }

    static RenderFrustumPlane planeOf(const Vec4& equation)
    {
        RenderFrustumPlane plane;
        plane.normal = Vec3(equation);
        plane.distance = equation.w;
        const float length = Math::Length(plane.normal);
        if (length > 0.0f)
        {
            plane.normal /= length;
            plane.distance /= length;
        }
        return plane;
    }
};

namespace RenderCulling
{
    inline bool aabbIntersectsSphere(const RenderAABB& bounds, const Vec3& center, float radius)
    {
        if (!bounds.valid)
            return true;

        const Vec3 closest = glm::clamp(center, bounds.min, bounds.max);
        const Vec3 delta = closest - center;
        return Math::Dot(delta, delta) <= radius * radius;
    }
}

#endif // !RenderCulling_hpp
