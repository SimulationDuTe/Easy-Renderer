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

	//创建立方体面索引数组
	int cubeFaces[12][3] = {   // 6 个面，每个面 2 个三角形
		{0,1,2},{0,2,3},  // 面0
		{4,6,5},{4,7,6},  // 面1
		{4,0,3},{4,3,7},  // 面2
		{1,5,6},{1,6,2},  // 面3
		{3,2,6},{3,6,7},  // 面4
		{4,5,1},{4,1,0},  // 面5
	};

	//方向光：固定方向。
	Vec3 lightDir = Vec3(0.5f, 1.0f, 1.0f).normalized();//单位向量
	//物体基础色
	Vec3 baseColor = Vec3(255, 255, 255); 
	//摄像机位置：固定在原点(0,0,0)，看向 -Z 方向
	Vec3 cameraPos = Vec3(2, 2, 2);
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
		Matrix4 model = Matrix4::RotationY(angle) * Matrix4::RotationX(angle * 0.5f);

		//视图矩阵.把立方体往后移到 z=-5（相机在原点看 -z）
		Matrix4 view = Matrix4::Translation(0, 0, -5);

		//投影矩阵.透视投影
		Matrix4 projection = Matrix4::perspective(60.0f, (float)W / H, 0.1f, 100.0f);

		Matrix4 projview = projection * view; //先视图再投影（model 单独用，为了拿世界坐标）

		for (int f = 0; f < 12; f++) 
		{
			//从模型空间到世界空间的顶点变换，获取世界坐标
			Vec3 w[3];
			for (int i = 0; i < 3; i++) 
				w[i] = model * cubeVerts[cubeFaces[f][i]];

			//面法线
			/*假设三角形ABC，要求法线，就得先有向量，B-A一个，C-A一个，然后这两个叉乘得到面法线,最后归一化为单位向量*/
			Vec3 normal = (w[2] - w[0]).cross(w[1] - w[0]).normalized();

			//面中心
			//三角形三个顶点的平均值，代表这个面的中心位置
			Vec3 faceCenter = (w[0] + w[1] + w[2]) * (1.0f / 3.0f);


			//计算从面中心到相机的方向向量，并归一化为单位向量
			Vec3 viewDir = (cameraPos - faceCenter).normalized();
			//计算半程向量，并归一化为单位向量
			Vec3 halfDir = (viewDir + lightDir).normalized();//半程向量 = 指向光源方向 + 指向视线方向，归一化
			
			/*三项光照*/
			//一：环境光：环境光强度 * 物体基础色
			Vec3 ambient = ambientStrength * baseColor;
			//二：漫反射：漫反射强度 * 物体基础色 * max(0, 法线·光源方向)
			Vec3 diffuse = diffuseStrength * baseColor * std::max(0.0f, normal.dot(lightDir));
			//三：镜面反射：镜面反射强度 * 高光颜色 * pow(max(0, 法线·半程向量), 光泽),钳制到0~1；
			float spec = std::powf(std::max(0.0f,normal.dot(halfDir)), shininess);
			Vec3 specular = specularStrength * white * spec;

			//最终颜色 = 环境光 + 漫反射 + 镜面反射
			Vec3 litColor = ambient + diffuse + specular;
	
			
			//变换到裁剪空间 → 透视除法 → 视口
			Vec3 tri[3];
			for (int i = 0; i < 3; i++)
			{
				Vec3 v = projview * w[i];
				v.PerspectiveDivision(); //透视除法 变换到NDC坐标[-1,1]
				//视口变换：NDC [-1, 1] → 屏幕坐标 [0, W] 和 [0, H]
				v.x = (v.x + 1) * 0.5f * W;
				v.y = (1 - v.y) * 0.5f * H;
				v.z = (v.z + 1) * 0.5f;
				tri[i] = v;
			}
			//三个顶点同一个颜色 = Flat（逐面）着色
			Vec3 col[3] = { litColor, litColor, litColor };
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