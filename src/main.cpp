#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>

#include "Buffer.h"

struct Vec3
{
	float x, y, z;
	Vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

	//标量乘
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
	//加法
	Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }

	friend inline Vec3 operator*(float s, const Vec3& v) {
		return Vec3(v.x * s, v.y * s, v.z * s);
	}
};

//画三角形
void DrawTriangle(ColorBuffer& colorbuffer, DepthBuffer& zbuffer, const SDL_PixelFormatDetails* fmt, Vec3 v[3], Vec3 c[3])
{
	//计算三角形包围盒，只是三角形的最小外接矩形，方便遍历像素
	int minX = std::max(0, (int)std::min({v[0].x, v[1].x, v[2].x}));
	int maxX = std::min(colorbuffer.Width() - 1, (int)std::max({ v[0].x, v[1].x, v[2].x }));
	int minY = std::max(0, (int)std::min({ v[0].y, v[1].y, v[2].y }));
	int maxY = std::min(colorbuffer.Height() - 1, (int)std::max({ v[0].y, v[1].y, v[2].y }));
	/*
	包围盒是矩形的，超出了实际三角形的面积，故而有点并不在三角形中，所以需要通过边函数来判断，这是为了优化，
	而判断产生的副产物——三角形面积比值（小三角形与整个三角形），则可以用来通过归一化计算重心坐标，因为重心坐标等于归一化的面积比值
	*/

	//遍历包围盒内每个像素
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			//计算边函数
			//e012为未归一化的权重，e0,e1,e2为点到边的距离
			//edge(A, B, P) = (B-A) × (P-A)
			Vec3 p(x, y, 0);
			int e0 = (v[2].x - v[1].x)*(p.y - v[1].y) - (v[2].y - v[1].y)*(p.x - v[1].x);
			int e1 = (v[0].x - v[2].x)*(p.y - v[2].y) - (v[0].y - v[2].y)*(p.x - v[2].x);
			int e2 = (v[1].x - v[0].x)*(p.y - v[0].y) - (v[1].y - v[0].y)*(p.x - v[0].x);

			//判断该点是否在三角形内
			bool inside = (e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0);
			if (!inside) continue;//不同号即在外

			//计算重心坐标(该点的权重)
			float area = e0 + e1 + e2;//未归一化的面积
			float w0 = e0 / area, w1 = e1 / area, w2 = e2 / area;//归一化的重心坐标

			//深度测试
			float z = w0 * v[0].z + w1 * v[1].z + w2 * v[2].z;
			if (z >= zbuffer(x, y)) continue;
			zbuffer(x, y) = z;

			//计算颜色插值
			Vec3 color = w0 * c[0] + w1 * c[1] + w2 * c[2];

			// 钳制颜色到 [0, 255]（防止溢出）
			color.x = std::max(0.0f, std::min(255.0f, color.x));
			color.y = std::max(0.0f, std::min(255.0f, color.y));
			color.z = std::max(0.0f, std::min(255.0f, color.z));

			// 写入帧缓冲
			colorbuffer(x, y) = SDL_MapRGBA(fmt, NULL,
				(Uint8)color.x,
				(Uint8)color.y,
				(Uint8)color.z,
				255);
		}
	}
}


int main(int argc, char* argv[])
{
	//初始化sdl窗口
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed: %s", SDL_GetError());
		return 1;
	}
	
	//创建窗口
	const int W = 641, H = 480;
	SDL_Window* window = SDL_CreateWindow("EasyRenderer", W, H, 0);
	if (!window)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateWindow failed: %s", SDL_GetError());
		return 1;
	}

	//创建渲染器
	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
	if (!renderer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateRenderer failed: %s", SDL_GetError());
		return 1;
	}

	//创建帧缓冲区
	SDL_Texture* Texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);

	//获取像素格式的描述
	const SDL_PixelFormatDetails* fmt =
		SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_XRGB8888);

	//用 Buffer 取代裸数组
	ColorBuffer framebuffer(W,H);


	// 在进入主循环前，先定义方块的位置和大小
	/*const int RECT_X = 50, RECT_Y = 50;
	const int RECT_W = 100, RECT_H = 100;*/

	//主循环
	bool done = false;
	while (!done)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
					done = true;
					break;
				default:
					break;
			}
		}

		//创建深度缓冲区
	DepthBuffer zbuffer(W, H);

		framebuffer.clear(0xFF000000);   // 颜色清黑
		zbuffer.clear(1.0f);             // 深度清"最远"


		//逐像素写渐变到FrameBuffer中
		/*for (int y = 0; y < H; y++)
		{
			for (int x = 0; x < W; x++)
			{
				framebuffer(x, y) = SDL_MapRGBA(fmt, NULL,
					(Uint8)(x * 255 / W),
					(Uint8)(y * 255 / H),
					128, 255);
			}
		}*/
		//for (int y = RECT_Y; y < RECT_Y + RECT_H; y++) {
		//	for (int x = RECT_X; x < RECT_X + RECT_W; x++) {
		//		// 确保不越界
		//		if (x >= 0 && x < W && y >= 0 && y < H) {
		//			framebuffer(x, y) = SDL_MapRGBA(fmt, NULL,
		//				(Uint8)(x * 255 / W),
		//				(Uint8)(y * 255 / H),
		//				64, 255);
		//		}
		//	}
		//}

		// 三角形1：RGB 渐变（较远，z=0.7）
		Vec3 t1v[3] = { Vec3(150,100,0.7f), Vec3(500,150,0.7f), Vec3(320,400,0.7f) };
		Vec3 t1c[3] = { Vec3(255,0,0), Vec3(0,255,0), Vec3(0,0,255) };
		DrawTriangle(framebuffer, zbuffer, fmt, t1v, t1c);

		// 三角形2：白色，与三角形1重叠，更近（z=0.3）
		Vec3 t2v[3] = { Vec3(320,50,0.3f), Vec3(600,450,0.3f), Vec3(100,450,0.3f) };
		Vec3 t2c[3] = { Vec3(255,255,255), Vec3(255,255,255), Vec3(255,255,255) };
		DrawTriangle(framebuffer, zbuffer, fmt, t2v, t2c);

		//锁定纹理，获取像素指针和pitch(行字节数)
		Uint32* pixels; int pitch;
		SDL_LockTexture(Texture, NULL, (void**)&pixels, &pitch);

		//上传到帧缓冲区，按字节数逐行上传（注意pitch）
		for (int y = 0; y < H; y++)
		{
			//因为pitch是字节数，而不是像素数，所以要用char*，按字节偏移
			memcpy((char*)pixels + y * pitch, framebuffer.data() + y * W, W * sizeof(uint32_t));
		}
		//解锁纹理
		SDL_UnlockTexture(Texture);


		//纹理渲染到屏幕
		SDL_RenderTexture(renderer, Texture, NULL, NULL);
		SDL_RenderPresent(renderer);


	}

	//清理
	SDL_DestroyTexture(Texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;


}