#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#include "Buffer.h"

struct Vec3
{
	float x, y, z;
	Vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

//画三角形
void DrawTriangle(ColorBuffer& buffer, Vec3 v0, Vec3 v1, Vec3 v2, uint32_t color)
{
	//计算三角形包围盒
	int minX = std::max(0, (int)std::min({v0.x, v1.x, v2.x}));
	int maxX = std::min(buffer.Width() - 1, (int)std::max({ v0.x, v1.x, v2.x }));
	int minY = std::max(0, (int)std::min({ v0.y, v1.y, v2.y }));
	int maxY = std::min(buffer.Height() - 1, (int)std::max({ v0.x,v1.x,v2.x }));


	//遍历包围盒内每个像素
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			//计算重心坐标
			int w0 = (v2.x - v1.x)*(y - v1.y) - (v2.y - v1.y)*(x - v2.x);
			int w1 = (v0.x - v2.x)*(y - v2.y) - (v0.y - v2.y)*(x - v0.x);
			int w2 = (v1.x - v0.x)*(y - v0.y) - (v1.y - v0.y)*(x - v1.x);

			//如果重心坐标都大于等于0，则像素在三角形内
			if(w0 >= 0 && w1 >= 0 && w2 >= 0)
			{
				buffer(x, y) = color;
			}
			else
			{
				//像素在三角形外，什么都不做
			}
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

		//先清空帧缓冲为背景色（例如黑色）
		framebuffer.clear(0xFF000000);


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

		//画三角形
		DrawTriangle(framebuffer,
			Vec3(100, 100, 0), Vec3(400, 120, 0), Vec3(250, 380, 0),
			SDL_MapRGBA(fmt, NULL, 255, 200, 0, 255));

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