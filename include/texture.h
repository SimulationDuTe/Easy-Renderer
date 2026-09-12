#pragma once

#include <vector>
#include <string>

#include "Vec3.h"
#include "../vendored/stb_image/stb_image.h"

class texture
{
public:
	texture(const std::string& path)
	{
		stbi_set_flip_vertically_on_load(true);//让 v=0 在底部（OpenGL 约定）,因为图片的（0，0）在左上角，渲染器的在左下角
		int w, h, ch;
		unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
		if (!data) {printf("Failed to load texture: %s\n", path.c_str()); return; }
		width = w; height = h;
		if (!data) { printf("Failed to load texture: %s\n", path.c_str()); return; }
		pixels.assign(data, data + w * h * 4);
		stbi_image_free(data);
	}

	//采样U，V->[0,1]，返回颜色 (0-255)
	Vec3 sample(float u, float v) const
	{
		//环绕模式处理UV坐标
		//把 u, v 映射到 [0, 1)。
		//先取“不大于原数的最大整数”，然后用原数减去
		u = u - floorf(u);
		v = v - floorf(v);

		//转换为像素坐标
		//u -> [0, 1) → u * width -> [0, width)
		//(int) 截断取整
		//% width 防止 u = 1.0 时越界
		int px = (int)(u * width) % width;
		int py = (int)(v * height) % height;

		//计算索引
		//py行个width的像素，用px定位，4个字节偏移
		int idx = (py * width + px) * 4;
		return Vec3(pixels[idx], pixels[idx + 1], pixels[idx + 2]); 
	}
	

private:

	int width = 0, height = 0;
	std::vector<unsigned char> pixels;
};


