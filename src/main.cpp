#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>

#include "Buffer.h"
#include "Vec3.h"
#include "Rasterizer.h"
#include "Matrix4.h"


struct Face { int v[3]; };
std::vector<Vec3>  sphereVerts, sphereNormals;
std::vector<Face>  sphereFaces;

const int latBands = 32, lonBands = 32;   // 经纬度分段数



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

	//创建球体顶点和面
	for (int lat = 0; lat <= latBands; lat++) 
	{
		float theta = lat * PI / latBands;              // 0..PI（北极→南极）
		float st = sinf(theta), ct = cosf(theta);
		for (int lon = 0; lon <= lonBands; lon++) 
		{
			float phi = lon * 2.0f * PI / lonBands;     // 0..2PI
			float sp = sinf(phi), cp = cosf(phi);
			Vec3 p(cp * st, ct, sp * st);                 // 单位球顶点
			sphereVerts.push_back(p);
			sphereNormals.push_back(p);                    // 单位球：法线 = 位置
		}
	}

	//创建索引面（每个四边形分成两个三角形）
	for (int lat = 0; lat < latBands; lat++)
		for (int lon = 0; lon < lonBands; lon++) 
		{
			int a = lat * (lonBands + 1) + lon;
			int b = a + lonBands + 1;
			sphereFaces.push_back({ a, b, a + 1 });
			sphereFaces.push_back({ b, b + 1, a + 1 });
		}


	//高光颜色
	Vec3 white = Vec3(255, 255, 255);
	//环境光强度
	float ambientStrength = 0.1f;
	//高光强度
	float specularStrength = 0.8f;
	//漫反射强度
	float diffuseStrength = 0.3f;
	//光泽
	float shininess = 16.0f;//通过powf，将高光收窄
	

	//旋转角度
	float angle = 0.0f;

	

	//视图矩阵.把立方体往后移到 z=-5（相机在原点看 -z）
	Matrix4 view = Matrix4::Translation(0, 0, -5);

	//投影矩阵.透视投影
	Matrix4 projection = Matrix4::perspective(60.0f, (float)W / H, 0.1f, 100.0f);

	//顶点着色器
	BlinnPhongShader shaderLeft;
	PhongShader shaderRight;

	shaderLeft.viewProj = projection * view;
	shaderLeft.lightDir = Vec3(0.5f, 1.0f, 1.0f).normalized();
	shaderLeft.cameraPos = Vec3(0, 0, 0);
	shaderLeft.baseColor = Vec3(255, 0, 0);     // 红球

	shaderRight.viewProj = projection * view;
	shaderRight.lightDir = Vec3(0.5f, 1.0f, 1.0f).normalized();
	shaderRight.cameraPos = Vec3(0, 0, 0);
	shaderRight.baseColor = Vec3(0, 0, 255);     // 蓝球




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
		//framebuffer.clear(0xFFFFFFFF);   // 清成白色
		zbuffer.clear(0.0f);             // 深度清"最远"

		angle += 0.01f; //每帧旋转角度增量

		/*
		局部坐标 (cubeVerts)
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

		// 模型矩阵：绕 Y 和 X 转
		//Matrix4 model = Matrix4::RotationY(angle) * Matrix4::RotationX(angle * 0.5f);



		Matrix4 mleft = Matrix4::Translation(-1.5f, 0, 0) * Matrix4::RotationY(angle);
		Matrix4 mright = Matrix4::Translation(1.5f, 0, 0) * Matrix4::RotationY(angle);



		//lambda:渲染整个球体
		auto DrawSphere = [&](Matrix4 model, Shader& shader)
			{
				for (auto& f : sphereFaces)
				{
					Vec3 worldPos[3], normal[3], clip[3];
					for (int i = 0; i < 3; ++i)
					{
						worldPos[i] = model * sphereVerts[f.v[i]];
						normal[i] = model.TransFormDir(sphereNormals[f.v[i]]);//忽略平移分量，法线不受平移影响
					}
					for (int i = 0; i < 3; ++i) 
						clip[i] = shader.Vertex(worldPos[i], normal[i], i);
					for (int i = 0; i < 3; ++i)
					{
						clip[i].PerspectiveDivision();
						clip[i].x = (clip[i].x + 1) * 0.5f * W;
						clip[i].y = (1 - clip[i].y) * 0.5f * H;
						clip[i].z = (clip[i].z + 1) * 0.5f;
					}
					Rasterizer::DrawTriangle(framebuffer, zbuffer, fmt, clip, shader);
				}
			};

		DrawSphere(mleft, shaderLeft);
		DrawSphere(mright, shaderRight);


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