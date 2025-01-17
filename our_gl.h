
#ifndef __OUR_GL_H__
#define __OUR_GL_H__

#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0); // coeff = -1/c;
void lookat(Vec3f eye, Vec3f center, Vec3f up);
float interpolate(float p1, float p2, float p3, float alpha, float beta, float gamma);
Vec3f barycentric(const Vec3f *pts, const Vec2f p);
Matrix convert_to_homo(Vec3f v);
Vec3f homo_to_vert(Matrix m);

struct IShader{
    virtual ~IShader();
    virtual Vec3f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

void triangle(Vec3f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer);
void Shading(Vec3f *pts, IShader &shader, TGAImage &image,float *zbuffer);

#endif //__OUR_GL_H__