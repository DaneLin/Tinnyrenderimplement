#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include "model.h"
//构造函数
/**
 * 20230725 author: lin
 * 1、更新了obj文件中f的读取，能够写入对应f的vert，tex_vert, norm_vert;
*/
Model::Model(const char *filename) : verts_(), faces_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) 
    {
        std::cerr << "Unable to open file!" << std::endl;
        return;
    }
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            verts_.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> vt;
            std::vector<int> vn;
            int idx_f, idx_vt, idx_vn;
            iss >> trash;
            while (iss >> idx_f >> trash >> idx_vt >> trash >> idx_vn) {
                idx_f--; // in wavefront obj all indices start at 1, not zero
                idx_vt--;
                idx_vn--;
                f.push_back(idx_f);
                vt.push_back(idx_vt);
                vn.push_back(idx_vn);
            }
            faces_.push_back(f);
            fuvs_.push_back(vt);
            fnorms_.push_back(vn);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f vt;
            float ftrash;
            for (int i = 0; i < 3; i++) 
            {
                if (i == 2)
                {
                    iss >> ftrash;
                    continue;
                } 
                iss >> vt.raw[i];
            }
            uvs_.push_back(vt);
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f vn;
            for (int i = 0; i < 3; i++) 
            {
                iss >> vn.raw[i];
            }
            norms_.push_back(vn);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
    read_model_texture(filename, "_diffuse.tga", diffuseMap);
}

Model::~Model() {
}

void Model::read_model_texture(std::string filename, const std::string suffix, TGAImage &img)
{
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) return;
    std::string curFileName = filename.substr(0, dot) + suffix;
    bool ok = img.read_tga_file(curFileName.c_str());
    if(ok) std::cout << "loaded texture successfully"<< std::endl;
    else std::cout << "读取纹理失败，检查文件名是否正确输入" << std::endl;
    img.flip_vertically();
}

//返回模型顶点个数
int Model::nverts() {
    return (int)verts_.size();
}
//返回模型表面个数
int Model::nfaces() {
    return (int)faces_.size();
}
//获取idx的表面
std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec2i Model::uv(int i) {
    
    return Vec2i(uvs_[i].x * diffuseMap.get_width() ,uvs_[i].y * diffuseMap.get_height());
}

Vec3f Model::norm(int i) {
    return norms_[i];
}

std::vector<int> Model::fuvs(int idx) {
    return fuvs_[idx];
}

std::vector<int> Model::fnorms(int idx) {
    return fnorms_[idx];
}

