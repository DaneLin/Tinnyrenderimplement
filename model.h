#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
	// std::vector<Vec3f> verts_;
	// std::vector<Vec2f> uvs_;
	// std::vector<Vec3f> norms_;
	// std::vector<std::vector<int>> faces_;//每个面是如何组成的，对应的v，vt，vn
	// std::vector<std::vector<int>> fuvs_;
	// std::vector<std::vector<int>> fnorms_;
	// TGAImage diffuseMap_;
	std::vector<Vec3f> verts_;
	std::vector<std::vector<Vec3i>> faces_;
	std::vector<Vec3f> norms_;
	std::vector<Vec2f> uv_;
	TGAImage diffuseMap_;
	void loadTexture(std::string filename, const char* suffix, TGAImage& image); 
public:
	Model(const char *filename);
	~Model();
	int nverts();//获取顶点个数
	int nfaces();//获取面的个数
	Vec3f vert(int i);
	Vec2f uv(int i);
	Vec3f norm(int i);
	std::vector<int> fuvs(int idx);
	std::vector<int> fnorms(int idx);
	std::vector<int> face(int idx);
	TGAColor diffuse(Vec2i uv);
	Vec2i uv(int iface, int nvert);
};

#endif //__MODEL_H__
