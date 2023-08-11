#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec2f> uvs_;
	std::vector<Vec3f> norms_;
	std::vector<std::vector<int> > faces_;//每个面是如何组成的，对应的v，vt，vn
	std::vector<std::vector<int> > fuvs_;
	std::vector<std::vector<int> > fnorms_; 
	TGAImage diffuseMap;
	TGAImage normalMap;
	TGAImage tangentMap;
	TGAImage specularMap;
public:
	Model(const char *filename);
	~Model();
	int nverts();//获取顶点个数
	int nfaces();//获取面的个数
	Vec3f vert(int i);
    Vec3f vert(int iface, int nthvert);
    Vec2i uv(int i);
	Vec2i uv(int iface, int nthvert);
    Vec3f norm(int iface, int nthvert);
    
    Vec3f norm(int i);
	std::vector<int> fuvs(int idx);
	std::vector<int> fnorms(int idx);
	std::vector<int> face(int idx);
	void read_model_texture(std::string filename, const std::string suffix, TGAImage &img);
	TGAColor diffuse(int x, int y) { return diffuseMap.get(x, y); };
	Vec3f normal(int x, int y);
	Vec3f tangent(int x, int y);
	float specular(int iface, int nthvert);
};

#endif //__MODEL_H__
