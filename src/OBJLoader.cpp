#include "OBJLoader.h"
#include <fstream>
#include <sstream>

bool loadOBJ(Mesh& mesh, const std::string& path)
{
    std::ifstream file(path);
    if (!file) return false;

    // Çå¿Õ¾ÉÊý¾Ý
    mesh.vertices.clear();
    mesh.Normals.clear();
    mesh.Textures.clear();
    mesh.Faces.clear();

    std::string line;
    while (std::getline(file, line))   
    {
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v")
        {
            float x, y, z;
            ss >> x >> y >> z;
            mesh.vertices.push_back(Vec3(x, y, z));
        }
        else if (type == "vn")
        {
            float x, y, z;
            ss >> x >> y >> z;
            mesh.Normals.push_back(Vec3(x, y, z));
        }
        else if (type == "vt")
        {
            float u, v;
            ss >> u >> v;
            mesh.Textures.push_back(Vec3(u, v, 0));
        }
        else if (type == "f")
        {
            Face face;
            for (int i = 0; i < 3; ++i)
            {
                std::string tok; ss >> tok;
                int vi = -1, ti = -1, ni = -1;
                size_t s1 = tok.find('/');
                if (s1 == std::string::npos)
                {
                    vi = std::stoi(tok) - 1;
                }
                else
                {
                    vi = std::stoi(tok.substr(0, s1)) - 1;
                    size_t s2 = tok.find('/', s1 + 1);
                    if (s2 == std::string::npos)
                    {
                        ti = std::stoi(tok.substr(s1 + 1)) - 1;
                    }
                    else
                    {
                        if (s2 > s1 + 1) ti = std::stoi(tok.substr(s1 + 1, s2 - s1 - 1)) - 1;
                        ni = std::stoi(tok.substr(s2 + 1)) - 1;
                    }
                }
                face.v[i] = vi; face.vt[i] = ti; face.vn[i] = ni;
            }
            mesh.Faces.push_back(face);
        }
    }
    return true;
}