#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>

#include "texture.h"
#include "Buffer.h"
#include "Vec3.h"
#include "Rasterizer.h"
#include "Matrix4.h"
#include "Camera.h"
#include "mesh.h"
#include "OBJLoader.h"






int main(int argc, char* argv[])
{
	//初始化sdl窗口
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed: %s", SDL_GetError());
		return 1;
	}
	
	//创建窗口
	const int W = 1280, H = 720;
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

	//创建着色器
	TextureShader shader;
	shader.lightDir = Vec3(0.0f, 0.0f, 1.0f).normalized();
	
	//创建相机
	Camera camera;

	

	//旋转角度
	float angle = 0.0f;

	


	Matrix4 view = camera.GetViewMatrix();
	
	

	//投影矩阵.透视投影
	Matrix4 projection = Matrix4::perspective(60.0f, (float)W / H, 0.1f, 100.0f);


	//加载模型
	Mesh mesh;
	if (!loadOBJ(mesh, "scenes/diablo3_pose.obj")) {
		printf("Failed to load OBJ\n");
		return 1;
	}

	//加载纹理
	texture tex("scenes/diablo3_pose_diffuse.png");

	shader.texture = &tex;

	//主循环
	bool done = false;

	bool dragging = false;

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
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					if (event.button.button == SDL_BUTTON_RIGHT) dragging = true;
					break;
				case SDL_EVENT_MOUSE_BUTTON_UP:
					if (event.button.button == SDL_BUTTON_RIGHT) dragging = false;
					break;
				case SDL_EVENT_MOUSE_MOTION:
					if (dragging)
					{
						camera.yaw += event.motion.xrel * 0.005f;
						camera.pitch += event.motion.yrel * 0.005f;
						if (camera.pitch > 1.5f) camera.pitch = 1.5f;
						if (camera.pitch < -1.5f) camera.pitch = -1.5f;
					}
					break;
				case SDL_EVENT_MOUSE_WHEEL:
					//乘性缩放，远近距离都平滑
					camera.distance *= (event.wheel.y > 0) ? 0.9f : 1.1f;
					if (camera.distance < 0.5f) camera.distance = 0.5f;
					if (camera.distance > 50.0f) camera.distance = 50.0f;
					break;
				default:
					break;
			}
		}

		//每帧更新
		Matrix4 view = camera.GetViewMatrix();
		shader.viewProj = projection * view;
		shader.cameraPos = camera.GetPosition();
		
		framebuffer.clear(0xFF000000);   // 颜色清黑
		//framebuffer.clear(0xFFFFFFFF);   // 清成白色
		zbuffer.clear(0.0f);             // 深度清"最远"

		angle += 0.01f; //每帧旋转角度增量

		/*
		局部坐标
		↓ 模型矩阵 (旋转)
		世界坐标		← 光照
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


		// 模型矩阵：绕 Y 轴旋转
		Matrix4 model = Matrix4::RotationY(angle);
		
		//遍历所有面
		for (auto& f : mesh.Faces)
		{
			Vec3 worldPos[3], worldNormal[3], worldUV[3], clip[3];
			for (int i = 0; i < 3; ++i)
			{
				//顶点：
				worldPos[i] = model * mesh.vertices[f.v[i]];

				//法线
				if (f.vn[i] >= 0)
				{
					worldNormal[i] = model.TransFormDir(mesh.Normals[f.vn[i]]);
				}
				else
				{
					//没有法线时用默认（面法线）代替
					Vec3 e1 = mesh.vertices[f.v[1]] - mesh.vertices[f.v[0]];
					Vec3 e2 = mesh.vertices[f.v[2]] - mesh.vertices[f.v[0]];
					worldNormal[i] = model.TransFormDir(e1.cross(e2).normalized());
				}	

				//纹理
				if (f.vt[i] >= 0)
					worldUV[i] = mesh.Textures[f.vt[i]];
				else
					worldUV[i] = Vec3(0,0,0);
			}

			//顶点着色器;
			for (int i = 0; i < 3; ++i)
				clip[i] = shader.Vertex(worldPos[i],worldNormal[i],worldUV[i],i);

			//透视除法 + 视口变换
			//NDC-> [-1, 1]
			//屏幕幕坐标范围是 [0, 1]（或 [0, W]、[0, H]）
			//+1 把 [-1, 1] 平移到 [0, 2]，*0.5 再缩放到 [0, 1]。两步合起来就是从 NDC 映射到 [0, 1]
			for (int i = 0; i < 3; i++)
			{
				clip[i].PerspectiveDivision();
				clip[i].x = (clip[i].x + 1) * 0.5f * W;
				clip[i].y = (1 - clip[i].y) * 0.5f * H;//y是反的
				clip[i].z = (clip[i].z + 1) * 0.5f;
			}
			Rasterizer::DrawTriangle(framebuffer, zbuffer, fmt, clip, shader);
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