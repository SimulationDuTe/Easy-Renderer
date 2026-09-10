#pragma once

struct Vec3
{
	float x, y, z, w;
	Vec3(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : x(_x), y(_y), z(_z), w(_w) {}

	//标量乘
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s,w); }
	//加法
	Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z,w + o.w); }
	//减法
	Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z, w - o.w); }


	//点乘(其实在衡量：两个向量方向有多一致。)
	//a·b = |a||b|cos(θ)。当a、b都是单位向量时，a·b = cos(θ)。
	/*
		小于 90°	方向大致相同 
		等于 90°	垂直（最常用） 
		大于 90°	方向大致相反 
	*/	
	float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }


	//叉乘
	/*\vec a * \vec b = (a2*b3 - a3* b2, a3*b1 - a1*b3, a1*b2 - a2*b1)*/
	//一个平面上，任意两个不共线的向量，叉乘一下，就得到法线
	Vec3 cross(const Vec3& o) const
	{
		return Vec3
		(
			y * o.z - z * o.y,
			z * o.x - x * o.z,
			x * o.y - y * o.x,
			1.0f
		);
	}

	//归一化
	//因为需要求单位向量，所以需要归一化
	//归一化就是把一个向量变成单位向量，也就是长度变成 1，方向不变（向量除以长度）
	//向量长度 = 分量的平方和开根号
	Vec3 normalized() const
	{
		float lenSq = x * x + y * y + z * z;
		if (lenSq < 1e-8f) return Vec3(0, 0, 0, 1.0f);
		float inv = 1.0f / sqrtf(lenSq);
		return Vec3(x * inv, y * inv, z * inv, 1.0f);//因为除法开销大，先求平方和的倒数，再乘以分量，效率更高
	}


	//透视除法
	// 变换到NDC坐标[-1,1]
	void PerspectiveDivision()
	{
		if (w != 0.0f)
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
