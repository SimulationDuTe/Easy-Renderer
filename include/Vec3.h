#pragma once

struct Vec3
{
	float x, y, z, w;
	Vec3(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : x(_x), y(_y), z(_z), w(_w) {}

	//标量乘
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s,1.0f); }
	//加法
	Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z,1.0f); }

	//透视除法（将齐次坐标转换为三维坐标）
	void PerspectiveDivision()
	{
		if (!w == 0.0f)
		{
			x /= w;
			y /= w;
			z /= w;
			w = 1.0f;//透视除法之后，w回到默认值，将三维坐标转换回齐次坐标，因为光栅化器只认齐次坐标，xyz
		}
	}


	//友元函数，允许标量在左侧
	friend inline Vec3 operator*(float s, const Vec3& v) {
		return Vec3(v.x * s, v.y * s, v.z * s,1.0f);
	}
};
