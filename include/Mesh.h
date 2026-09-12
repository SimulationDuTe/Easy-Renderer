#pragma once 

#include "Vec3.h"
#include <vector>
#include <string>

//面
struct Face
{
	int v[3];//顶点
	int vt[3];//纹理
	int vn[3];//法线
};

struct Mesh
{
	std::vector<Vec3> vertices;
	std::vector<Vec3> Textures;
	std::vector<Vec3> Normals;
	std::vector<Face> Faces;
};