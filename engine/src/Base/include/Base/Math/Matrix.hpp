#ifndef Matrix_hpp
#define Matrix_hpp

#include "Vector.hpp"

using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

namespace Math
{

    inline Mat4 LookAt(const Vec3 &eye, const Vec3 &center, const Vec3 &up)
    {
        return glm::lookAt(eye, center, up);
    }

    inline Mat4 Perspective(float fovy, float aspect_ratio, float zNear, float zFar)
    {
        return glm::perspective(fovy, aspect_ratio, zNear, zFar);
    }

    inline Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        return glm::ortho(left, right, bottom, top, zNear, zFar);
    }

    inline Mat4 Scale(const Vec3 &scale)
    {
        return glm::scale(Mat4(1.0f), scale);
    }

    inline Mat4 Rotate(float angle, const Vec3 &rotate_axis)
    {
        return glm::rotate(Mat4(1.0f), angle, rotate_axis);
    }

    inline Mat4 Translate(const Vec3 &translation)
    {
        return glm::translate(Mat4(1.0f), translation);
    }

    inline void ShearX();
    inline void ShearY();
    inline void ShearZ();

    template <typename Mat>
    Mat Inverse(const Mat &mat)
    {
        return glm::inverse(mat);
    }

    inline float Determinant(const Mat4 &matrix)
    {
        return glm::determinant(matrix);
    }

    inline void SVDDecomposition();

    inline void DecomposeMatrix(const Mat4 &matrix, Mat4 &translation, Mat4 &rotation, Mat4 &scale)
    {
        scale = Mat4(1.0f);
        scale[0][0] = Math::Length(matrix[0]);
        scale[1][1] = Math::Length(matrix[1]);
        scale[2][2] = Math::Length(matrix[2]);

        rotation = Mat4(1.0f);
        rotation[0][0] = (matrix[0][0] / scale[0][0]);
        rotation[0][1] = (matrix[0][1] / scale[0][0]);
        rotation[0][2] = (matrix[0][2] / scale[0][0]);
        rotation[0][3] = 0.f;
        rotation[1][0] = (matrix[1][0] / scale[1][1]);
        rotation[1][1] = (matrix[1][1] / scale[1][1]);
        rotation[1][2] = (matrix[1][2] / scale[1][1]);
        rotation[1][3] = 0.f;
        rotation[2][0] = (matrix[2][0] / scale[2][2]);
        rotation[2][1] = (matrix[2][1] / scale[2][2]);
        rotation[2][2] = (matrix[2][2] / scale[2][2]);
        rotation[2][3] = 0.f;
        rotation[3][0] = 0.f;
        rotation[3][1] = 0.f;
        rotation[3][2] = 0.f;
        rotation[3][3] = 1.0f;

        translation = Mat4(1.0f);
        translation[3][0] = matrix[3][0];
        translation[3][1] = matrix[3][1];
        translation[3][2] = matrix[3][2];
    }

    inline void rotationMatrixToAxisAngle(const Mat4 &R, Vec3 &outAxis, float &outAngle)
    {
        // 1. 将旋转矩阵转换为四元数
        // quat_cast 内部已经完美处理了矩阵迹为负数等各种边界情况
        glm::quat q = glm::quat_cast(R);

        // 2. 直接从四元数中提取旋转角和旋转轴
        outAngle = rad2deg(glm::angle(q));
        outAxis = glm::axis(q);
    }

    inline void rotationMatrixToEulerAngle(const glm::mat4 &R, float &x, float &y, float &z)
    {
        // 提取 XYZ 顺序的角度
        Vec3 ret = glm::eulerAngles(glm::quat_cast(R));
        x = rad2deg(ret.x);
        y = rad2deg(ret.y);
        z = rad2deg(ret.z);
    }

    inline Mat4 composeMatrix(const Vec3 &S, const Vec3 &R, const Vec3 &T)
    {
        glm::qua<float> rotation_qua = glm::qua<float>(Vec3(Math::deg2rad(R.x), Math::deg2rad(R.y), Math::deg2rad(R.z)));
        Mat4 rotation = glm::mat4_cast(rotation_qua);
        Mat4 result = Math::Translate(T) * rotation * Math::Scale(S);
        return result;
    }

}

#endif