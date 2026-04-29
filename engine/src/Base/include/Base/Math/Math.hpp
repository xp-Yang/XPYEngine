#ifndef Math_hpp
#define Math_hpp

#include <cmath>
#include <random>
#include <array>
#include <memory>
#include <map>
#include <unordered_map>
#include "Vector.hpp"
#include "Matrix.hpp"

namespace Math
{

    inline float randomUnit()
    {
        static std::uniform_real_distribution<double> distribution(0.0, 1.0);
        std::mt19937 generator(std::random_device{}());
        return distribution(generator);
        //// Returns a random real in [0,1).
        // return rand() / (RAND_MAX + 1.0);
    }
    inline float random(float min, float max)
    {
        // Returns a random real in [min,max).
        return min + (max - min) * randomUnit();
    }
    // generate a vector in a unit sphere
    inline Vec3 randomInUnitSphere()
    {
        while (true)
        {
            Vec3 unit_cube(random(-1, 1), random(-1, 1), random(-1, 1));
            if (Dot(unit_cube, unit_cube) < 1.0f)
            { // 为了均匀分布，否则归一化后沿立方体对角线的抽样比较多
                return unit_cube;
            }
        }
    }
    // generate a vector on a unit sphere
    inline Vec3 randomUnitVec()
    {
        return Normalize(randomInUnitSphere());
    }
    inline Vec3 randomOnHemisphere(const Vec3 &normal)
    {
        Vec3 unit = randomUnitVec();
        if (Dot(unit, normal) > 0.0)
            return unit;
        else
            return -unit;
    }

    inline Point3 getPointOnUnitSphere(const Point3 &plane_point, const Vec3 &normal)
    {
        float r_square = Dot(plane_point, plane_point);
        if (r_square > 1)
            return {};

        float h = std::sqrt(1 - r_square);

        return plane_point + h * normal;
    }
    // 单位圆内的点按极轴均匀分布(非均匀分布)
    inline Vec2 randomInUnitCircleByPolar()
    {
        float r = randomUnit();
        float theta = random(0, 2 * Constant::PI);
        return Vec2(r * cos(theta), r * sin(theta));
    }
    inline Vec3 randomLambertianDistribution(const Vec3 &normal)
    {
        // 1. 构造切平面
        Vec3 up = Vec3(0, 1, 0);
        Vec3 local_u;
        if (normal == up)
            local_u = Vec3(1, 0, 0);
        else
            local_u = Cross(normal, up);
        Vec3 local_v = Cross(normal, local_u);

        // 2. 在切平面的单位圆内均匀取点
        Point3 random_plane_point;
        Vec2 coef = randomInUnitCircleByPolar();
        random_plane_point = coef.x * local_u + coef.y * local_v;

        // 3. 将点映射回球面
        return getPointOnUnitSphere(random_plane_point, normal);
    }

    template <typename T>
    bool isApproxTo(const T &val, const T &to_val, double tolerance = Constant::epsilon)
    {
        return std::fabs((double)val - (double)to_val) < tolerance;
    }

    inline bool isApproxZero(float real)
    {
        return isApproxTo(real, 0.0f);
    }

    inline bool isApproxZero(const Vec3 &vec)
    {
        return isApproxZero(vec.x) && isApproxZero(vec.y) && isApproxZero(vec.z);
    }

    template <typename T>
    T lerp(const T &start, const T &end, float t)
    {
        return (1 - t) * start + t * end;
    }

    struct Interval
    {
        double min, max;

        Interval() : min(+FLT_MAX), max(-FLT_MAX) {} // Default interval is empty
        Interval(double _min, double _max) : min(_min), max(_max) {}
        bool contains(double x) const
        {
            return min <= x && x <= max;
        }
        bool surrounds(double x) const
        {
            return min < x && x < max;
        }
    };

}

#endif // !MathHpp
