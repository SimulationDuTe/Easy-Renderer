#pragma once

#include "Vec3.h"
#include "Matrix4.h"

#include <algorithm>


struct Shader
{
	virtual ~Shader(){}

	//顶点着色器:变换顶点 + 计算 per-vertex 数据，返回裁剪空间坐标
	virtual Vec3 Vertex(const Vec3& pos, const Vec3& normal, int index) = 0;

	//片段着色器:计算像素颜色，返回颜色值
	virtual Vec3 Fragment(float u, float v) = 0;
};


struct BlinnPhongShader : public Shader
{
	//uniform（每帧设置一次)
	Matrix4 viewProj;//
	Vec3 lightDir; //光源方向
	Vec3 cameraPos; //相机位置
	Vec3 baseColor;
	//环境光强度
	float ambientStrength = 0.1f;
	//高光强度
	float specularStrength = 0.8f;
	//高光颜色
	Vec3 specColor = Vec3(255, 255, 255);
	//漫反射强度
	float diffuseStrength = 0.3f;
	//光泽
	float shininess = 32.0f;//通过powf，将高光收窄

	//varing 每个顶点的属性（每个顶点都要计算一次）
	Vec3 varyingColor[3];

	//顶点着色器
	Vec3 Vertex(const Vec3& pos, const Vec3& normal, int index) override//必须是世界坐标和世界法线
	{
		Vec3 viewDir = (cameraPos - pos).normalized();
		Vec3 halfDir = (lightDir + viewDir).normalized();

		Vec3 ambient = baseColor * ambientStrength;
		Vec3 diffuse = baseColor * diffuseStrength * std::max(0.0f, normal.dot(lightDir));
		float spec = std::powf(std::max(0.0f, normal.dot(halfDir)), shininess);
		Vec3 specular = specColor * specularStrength * spec;

		varyingColor[index] = specular + diffuse + ambient;
		return viewProj * pos;
	}


	Vec3 Fragment(float u, float v) override
	{
		// 重心坐标插值：w0=1-u-v, w1=u, w2=v
		return varyingColor[0] * (1 - u - v) + varyingColor[1] * u + varyingColor[2] * v;
	}
};


struct PhongShader : public Shader
{
	//uniform
	Matrix4 viewProj;//
	Vec3 lightDir; //光源方向
	Vec3 cameraPos; //相机位置
	Vec3 baseColor;
	//环境光强度
	float ambientStrength = 0.1f;
	//高光强度
	float specularStrength = 0.8f;
	//高光颜色
	Vec3 specColor = Vec3(255, 255, 255);
	//漫反射强度
	float diffuseStrength = 0.3f;
	//光泽
	float shininess = 32.0f;//通过powf，将高光收窄


	//varying:法线和位置
	Vec3 varyingNormal[3], varyingPos[3];

	Vec3 Vertex(const Vec3& pos, const Vec3& normal, int index) override
	{
		varyingNormal[index] = normal;
		varyingPos[index] = pos;
		return viewProj * pos;//返回世界坐标
	}

	Vec3 Fragment(float u, float v) override
	{
		//插值法线和位置
		Vec3 normal = (varyingNormal[0] * (1 - u - v) + varyingNormal[1] * u + varyingNormal[2] * v).normalized();
		Vec3 pos = varyingPos[0] * (1 - u - v) + varyingPos[1] * u + varyingPos[2] * v;

		//半程向量
		Vec3 viewDir = (cameraPos - pos).normalized();
		Vec3 halfDir = (viewDir + lightDir).normalized();

		Vec3 ambient = ambientStrength * baseColor;
		Vec3 diffuse = diffuseStrength * baseColor * std::max(0.0f, normal.dot(lightDir));
		float spec = std::powf(std::max(0.0f, normal.dot(halfDir)), shininess);
		Vec3 specular = specularStrength * specColor * spec;

		return ambient + diffuse + specular;
	}

};