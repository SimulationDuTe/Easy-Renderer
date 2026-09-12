#pragma once

#include "Matrix4.h"
#include "Vec3.h"

//环绕相机：用球坐标（yaw / pitch / distance）描述相机位置
struct Camera
{
	Vec3 target = Vec3(0, 0, 0);	//相机看向的目标点
	float yaw = 0.0f;				//相机绕Y轴旋转的角度（水平旋转） 
	float pitch = 0.0f;            // 相机绕X轴旋转的角度（垂直旋转）
	float distance = 5.0f;            // 相机与目标点的距离

	//球坐标——>笛卡尔坐标（相机的世界位置）
	Vec3 GetPosition() const
	{
		float cosPitch = std::cosf(pitch); float sinPitch = std::sinf(pitch);
		float cosYaw = std::cosf(yaw); float sinYaw = std::sinf(yaw);
		//x：水平方向的左右分量
		//y: 垂直方向的上下分量
		//z:水平方向的前后分量
		//+target:从目标点出发，沿该方向走 distance，到达相机位置
		//* ditance： 把单位向量放大到相机距离
		return target + Vec3(cosPitch * sinYaw, sinPitch, cosYaw * cosPitch) * distance;
	}

	//由位置 + 朝向生成视图矩阵
	Matrix4 GetViewMatrix() const
	{
		return Matrix4::LookAt(GetPosition(), target, Vec3(0, 1, 0));
	}
};