#pragma once

#include <cmath>

#include "Vec3.h"

static constexpr float PI = 3.14159265358979323846f;

/*
给每个点添加第 4 个分量 w，通常设为 1，即 (x, y, z, 1)。然后用 4×4 矩阵的第 4 列存放平移量：
[1  0  0  dx]   [x]   [x + dx]
[0  1  0  dy] × [y] = [y + dy]
[0  0  1  dz]   [z]   [z + dz]
[0  0  0  1 ]   [1]   [1     ]
*/

struct Matrix4
{
	float matrix[4][4] = {}; // m[row][col]，默认全 0

	//单位矩阵(什么都不做的变换：I × v = v)
	//用于构建其他变换的起点（平移、旋转都从单位矩阵开始修改）。)
	static Matrix4 Identity()
	{
		Matrix4 r;
		for (int i = 0; i < 4; ++i)
			r.matrix[i][i] = 1.0f;
		return r;
	}

	//沿X轴旋转的变换矩阵
	/*
	[1    0     0    0]
    [0   cosθ  -sinθ  0]
	[0   sinθ   cosθ  0]
    [0    0     0    1]
	*/
	static Matrix4 RotationX(float angle)
	{
		Matrix4 r = Identity();
		float cosAngle = std::cos(angle);
		float sinAngle = std::sin(angle);
		r.matrix[1][1] =cosAngle, r.matrix[1][2] = -sinAngle;
		r.matrix[2][1] =sinAngle, r.matrix[2][2] = cosAngle;
		return r;
	}

	//沿Y轴旋转的变换矩阵
	/*
	[ cosθ  0  sinθ  0]
	[  0    1   0    0]
	[-sinθ  0  cosθ  0]
	[  0    0   0    1]
	*/
	static Matrix4 RotationY(float angle)
	{
		Matrix4 r = Identity();
		float cosAngle = std::cosf(angle);
		float sinAngle = std::sinf(angle);
		r.matrix[0][0] = cosAngle,r.matrix[0][2] = sinAngle;
		r.matrix[2][0] = -sinAngle, r.matrix[2][2] = cosAngle;
		return r;
	}

	//沿Z轴旋转的变换矩阵
	/*
	 [cosθ  -sinθ  0  0]
	 [sinθ   cosθ  0  0]
     [  0      0   1  0]
     [  0      0   0  1]
	*/
	static Matrix4 RotationZ(float angle)
	{
		Matrix4 r = Identity();
		float cosAngle = std::cosf(angle);
		float sinAngle = std::sinf(angle);
		r.matrix[0][0] = cosAngle, r.matrix[0][1] = -sinAngle;
		r.matrix[1][0] = sinAngle, r.matrix[1][1] = cosAngle;
		return r;
	}

	//平移矩阵
	/*
	[1  0  0  0]		[1  0  0  dx]
	[0  1  0  0]   ->   [0  1  0  dy]
	[0  0  1  0]		[0  0  1  dz]
	[0  0  0  1]		[0  0  0  1 ]
	第 4 列（索引 3）的前三个分量就是平移量 (dx, dy, dz)
	*/
	static Matrix4 Translation(float dx, float dy, float dz)
	{
		Matrix4 r = Identity();
		r.matrix[0][3] = dx;
		r.matrix[1][3] = dy;
		r.matrix[2][3] = dz;
		return r;
	}

	//透视投影矩阵
	//参数：FOV,屏幕宽高比，近裁剪面，远裁剪面
	static Matrix4 perspective(float fovDeg, float aspect, float n, float f) {
		float t = 1.0f / tanf(fovDeg * 0.5f * (float)PI/ 180.0f);  // 焦距
		Matrix4 r;
		r.matrix[0][0] = t / aspect;
		r.matrix[1][1] = t;
		r.matrix[2][2] = (f + n) / (f - n);// 透视投影矩阵的关键，z值映射到 [0,1]，并且 z 越小越远
		r.matrix[2][3] = (2.0f * f * n) / (f - n);//适配方向z，无负号
		r.matrix[3][2] = -1.0f;   // ★ 关键：把 -z 塞进 w，透视除法的来源
		return r;
	}

	//矩阵 * 向量
	Vec3 operator*(const Vec3& v) const
	{
		return Vec3
		(
			matrix[0][0] * v.x + matrix[0][1] * v.y + matrix[0][2] * v.z + matrix[0][3] * v.w,
			matrix[1][0] * v.x + matrix[1][1] * v.y + matrix[1][2] * v.z + matrix[1][3] * v.w,
			matrix[2][0] * v.x + matrix[2][1] * v.y + matrix[2][2] * v.z + matrix[2][3] * v.w,
			matrix[3][0] * v.x + matrix[3][1] * v.y + matrix[3][2] * v.z + matrix[3][3] * v.w
		);
	}

	//矩阵 * 矩阵(先应用x,再用this)
	// 公式：r[i][j] = Σ(k=0→3) m[i][k] × o[k][j]
	//“右操作数先应用”：T × R：先旋转，再平移。
	//不满足交换律：A × B ≠ B × A
	//但满足结合律：(A × B) × C = A ×(B × C)
	Matrix4 operator*(const Matrix4& o) const
	{
		Matrix4 r;
		for (int i = 0; i < 4; i++) //遍历结果矩阵的行
			for (int j = 0; j < 4; j++) //遍历结果矩阵的列
				for (int k = 0; k < 4; k++)
					r.matrix[i][j] += matrix[i][k] * o.matrix[k][j];
		return r;
	}

	Vec3 TransFormDir(const Vec3& v) const
	{
		return Vec3(
			matrix[0][0] * v.x + matrix[0][1] * v.y + matrix[0][2] * v.z,
			matrix[1][0] * v.x + matrix[1][1] * v.y + matrix[1][2] * v.z,
			matrix[2][0] * v.x + matrix[2][1] * v.y + matrix[2][2] * v.z);
	}

	Vec3 TransformPoint(const Vec3& v) const
	{
		return Vec3(
			matrix[0][0] * v.x + matrix[0][1] * v.y + matrix[0][2] * v.z + matrix[0][3],
			matrix[1][0] * v.x + matrix[1][1] * v.y + matrix[1][2] * v.z + matrix[1][3],
			matrix[2][0] * v.x + matrix[2][1] * v.y + matrix[2][2] * v.z + matrix[2][3]);
	}
};