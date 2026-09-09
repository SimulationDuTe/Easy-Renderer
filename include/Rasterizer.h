#pragma once

#include <SDL3/SDL.h>

#include <algorithm>

#include "Buffer.h"
#include "Vec3.h"

class Rasterizer
{
public:

	//边函数(p在a-b间哪一点)
	//edge(A, B, P) = (B-A) × (P-A)
	static float Edge(const Vec3& A, const Vec3& B, const Vec3& P)
	{
		return (B.x - A.x) * (P.y - A.y) - (B.y - A.y) * (P.x - A.x);
	}

	static void DrawTriangle(ColorBuffer& fb, DepthBuffer& zb, const SDL_PixelFormatDetails* fmt, const Vec3 v[3], const Vec3 c[3])
	{
		/*
		*包围盒是矩形的，超出了实际三角形的面积，故而有点并不在三角形中，所以需要通过边函数来判断，这是为了优化，
		*而判断产生的副产物——三角形面积比值（小三角形与整个三角形），则可以用来通过归一化计算重心坐标，因为重心坐标等于归一化的面积比值
		*/
		//计算三角形包围盒，只是三角形的最小外接矩形，方便遍历像素
		int minX = std::max(0, (int)std::min({ v[0].x, v[1].x, v[2].x }));
		int maxX = std::min(fb.Width() - 1, (int)std::max({ v[0].x, v[1].x, v[2].x }));
		int minY = std::max(0, (int)std::min({ v[0].y, v[1].y, v[2].y }));
		int maxY = std::min(fb.Height() - 1, (int)std::max({ v[0].y, v[1].y, v[2].y }));


		// 如果包围盒无效，直接返回
		if (minX > maxX || minY > maxY) return;

		for (int y = minY; y <= maxY; y++)
		{
			for (int x = minX; x <= maxX; x++)
			{
				Vec3 p(x, y, 0);
				//假设顶点顺序 v[0]→v[1]→v[2] 是逆时针
				//顺序可更改，但不可混用，否则会出现负面积
				//e012为未归一化的权重，e0,e1,e2为点到边的距离
				float e0 = Edge(v[1], v[2], p);
				float e1 = Edge(v[2], v[0], p);
				float e2 = Edge(v[0], v[1], p);

				//判断点是否在三角形内(包含边界)
				bool inside = (e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0);
				if (!inside) continue;

				//计算重心坐标(该点的权重)
				float area = e0 + e1 + e2;//未归一化的面积
				float w0 = e0 / area, w1 = e1 / area, w2 = e2 / area;//归一化的重心坐标

				//深度插值及深度测试
				float z = w0 * v[0].z + w1 * v[1].z + w2 * v[2].z;
				if (z <= zb(x, y)) continue;//z 越小越远 → 被挡住
				zb(x, y) = z;

				//计算颜色插值
				Vec3 color = w0 * c[0] + w1 * c[1] + w2 * c[2];

				// 钳制颜色到 [0, 255]（防止溢出）
				color.x = std::max(0.0f, std::min(255.0f, color.x));
				color.y = std::max(0.0f, std::min(255.0f, color.y));
				color.z = std::max(0.0f, std::min(255.0f, color.z));

				// 写入帧缓冲
				fb(x, y) = SDL_MapRGBA(fmt, NULL,
					(Uint8)color.x,
					(Uint8)color.y,
					(Uint8)color.z,
					255);
			}
		}
	}
};
