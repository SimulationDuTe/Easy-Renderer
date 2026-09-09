#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>

#include "Buffer.h"
#include "Vec3.h"
#include "Rasterizer.h"
#include "Matrix4.h"

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

	//创建深度缓冲区
	DepthBuffer zbuffer(W, H);

	//创建立方体顶点和面索引数据
	Vec3 cubeVerts[8] = {
	{-0.5,-0.5,-0.5}, { 0.5,-0.5,-0.5}, { 0.5, 0.5,-0.5}, {-0.5, 0.5,-0.5},  // 后 4 点
	{-0.5,-0.5, 0.5}, { 0.5,-0.5, 0.5}, { 0.5, 0.5, 0.5}, {-0.5, 0.5, 0.5}   // 前 4 点
	};
	int cubeFaces[12][3] = {   // 6 个面，每个面 2 个三角形
		{0,1,2},{0,2,3},  // 面0
		{4,6,5},{4,7,6},  // 面1
		{4,0,3},{4,3,7},  // 面2
		{1,5,6},{1,6,2},  // 面3
		{3,2,6},{3,6,7},  // 面4
		{4,5,1},{4,1,0},  // 面5
	};

	//创建顶点颜色数据
	Vec3 faceColors[6] = {
	{255,0,0},{0,255,0},{0,0,255},{255,255,0},{255,0,255},{0,255,255}
	};

	float angle = 0.0f; //旋转角度

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


		framebuffer.clear(0xFF000000);   // 颜色清黑
		zbuffer.clear(0.0f);             // 深度清"最远"

		angle += 0.01f; //每帧旋转角度增量

		/*
		局部坐标 (cubeVerts)
		↓ 模型矩阵 (旋转)
		世界坐标
		↓ 视图矩阵 (平移 -5)
		相机坐标
		↓ 投影矩阵 (透视)
		裁剪坐标 (x, y, z, w)   ← w = -z_camera
		↓ 透视除法 (x/w, y/w, z/w)
		NDC 坐标 (范围 [-1, 1])
		↓ 视口变换
		屏幕坐标 (x ∈ [0, W], y ∈ [0, H], z ∈ [0, 1])
		↓ DrawTriangle
		帧缓冲像素
		*/

		// 模型矩阵：绕 Y 和 X 转
		Matrix4 model = Matrix4::RotationY(angle) * Matrix4::RotationX(angle * 0.5f);

		//视图矩阵.把立方体往后移到 z=-5（相机在原点看 -z）
		Matrix4 view = Matrix4::Translation(0, 0, -5);

		//投影矩阵.透视投影
		Matrix4 projection = Matrix4::perspective(60.0f, (float)W / H, 0.1f, 100.0f);
		
		//Mvp矩阵（顺序从后到前）
		Matrix4 mvp = projection * view * model;

		for (int f = 0; f < 12; f++) {
			Vec3 tri[3], col[3];
			for (int i = 0; i < 3; i++) {
				//获取顶点索引
				int idx = cubeFaces[f][i];
				Vec3 v = mvp * cubeVerts[idx];     // ① 变换到裁剪空间（w 有意义了）
				v.PerspectiveDivision();             // ② 透视除法 → NDC
				v.x = (v.x + 1) * 0.5f * W;        // ③ 视口变换
				v.y = (1 - v.y) * 0.5f * H;        //    ★ y 要翻转（NDC 上=正，屏幕下=正）
				v.z = (v.z + 1) * 0.5f;            //    z 从 [-1,1] 映射到 [0,1]
				tri[i] = v;
				col[i] = faceColors[f / 2];        // 每面一种颜色
			}
			Rasterizer::DrawTriangle(framebuffer, zbuffer, fmt, tri, col);
		}

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