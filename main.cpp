#include "tgaimage.h"
#include <vector>
#include <cmath>
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include <algorithm>
#include <iostream>

const int width = 800;
const int height = 800;
Model *model = NULL;
const int depth = 255;
const char *model_path = "../obj/african_head.obj";
const char *image_path = "../output/output_african_head_tangent.tga";

Vec3f cameraPos(0, 0, 5);
Vec3f centerPos(0, 0, 0);
Vec3f up(0, 1, 0);
Vec3f light_dir = Vec3f(1, 1, 1).normalize();

struct GouraudShader : public IShader {
    Vec3f varying_intensity;

    virtual Vec3f vertex(int iface, int nthvert)
    {
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));
        varying_intensity[nthvert] = std::max(0.f, model->norm(iface, nthvert) * light_dir);
        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        float intensity = varying_intensity * bar;
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

struct TextureShader : public IShader {
    Vec3f varying_intensity;
    Matrix varying_uv = Matrix(2, 3);
    

    virtual Vec3f vertex(int iface, int nthvert)
    {
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));
        varying_intensity[nthvert] = std::max(0.f, model->norm(iface, nthvert) * light_dir);
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        Matrix tmp(3, 1);
        for (int i = 0; i < 3; i++) tmp[i][0] = bar[i];
        float intensity = varying_intensity * bar;
        Matrix uv = varying_uv * tmp;
        color = model->diffuse(uv[0][0], uv[1][0]) * intensity;
        return false;
    }
};

//interpret RGB values as xyz direction
struct NormalMappintShader : public IShader {
    Matrix varying_uv = Matrix(2, 3);
    Vec3f varying_nm;

    virtual Vec3f vertex(int iface, int nthvert)
    {
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));
        varying_uv.set_col(nthvert, model->uv(iface, nthvert)); 
        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        Matrix tmp(3, 1);
        for (int i = 0; i < 3; i++) tmp[i][0] = bar[i];
        Matrix uv = varying_uv * tmp;
        varying_nm = model->normal(uv[0][0], uv[1][0]).normalize();
        auto intensity = varying_nm * light_dir;
        color = model->diffuse(uv[0][0], uv[1][0]) * intensity;
        return false;
    }
};

//使用法线空间法线贴图
struct NormalTangentShader : public IShader {
    Matrix varying_uv = Matrix(2, 3);
    Vec3f view_tri[3];//3x3的矩阵，用来存储屏幕坐标下的三角形顶点
    Vec3f varying_nm;

    virtual Vec3f vertex(int iface, int nthvert)
    {
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));
        varying_uv.set_col(nthvert, model->uv(iface, nthvert)); 
        view_tri[nthvert] = gl_vert;
        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        Matrix tmp(3, 1);
        for (int i = 0; i < 3; i++) tmp[i][0] = bar[i];
        Matrix uv = varying_uv * tmp;//插值计算

        auto e1 = view_tri[2] - view_tri[0];
        auto e2 = view_tri[1] - view_tri[0];
        Vec2f deltaUV1 = Vec2f(varying_uv[0][2] - varying_uv[0][0], varying_uv[1][2] - varying_uv[0][1]);
        Vec2f deltaUV2 = Vec2f(varying_uv[0][1] - varying_uv[0][0], varying_uv[1][1] - varying_uv[0][1]);
        
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        Vec3f tg1, tg2;
        tg1.x = f * (deltaUV2.y * e1.x - deltaUV1.y * e2.x);
        tg1.y = f * (deltaUV2.y * e1.y - deltaUV1.y * e2.y);
        tg1.z = f * (deltaUV2.y * e1.z - deltaUV1.y * e2.z);
        tg1.normalize();

        tg2.x = f * (-deltaUV2.x * e1.x + deltaUV1.x * e2.x);
        tg2.y = f * (-deltaUV2.x * e1.y + deltaUV1.x * e2.y);
        tg2.z = f * (-deltaUV2.x * e1.z + deltaUV1.x * e2.z);
        tg2.normalize();

        Vec3f norm = tg1 ^ tg2;

        Matrix TBN(3, 3);
        TBN[0][0] = tg1.x;
        TBN[0][1] = tg1.y;
        TBN[0][2] = tg1.z;
        TBN[1][0] = tg2.x;
        TBN[1][1] = tg2.y;
        TBN[1][2] = tg2.z;
        TBN[2][0] = norm.x;
        TBN[2][1] = norm.y;
        TBN[2][2] = norm.z;

        varying_nm = model->tangent(uv[0][0], uv[1][0]).normalize();
        Matrix m_nm = Matrix(3,1);
        m_nm[0][0] = varying_nm.x;
        m_nm[1][0] = varying_nm.y;
        m_nm[2][0] = 1;

        Matrix tmp1 = TBN.transpose() * m_nm;
        Vec3f nm(tmp1[0][0], tmp1[1][0], tmp1[2][0]);
        
        auto intensity = nm* light_dir;
        color = model->diffuse(uv[0][0], uv[1][0]) * intensity;
        return false;
    }
};

int main(int argc, char **argv)
{
    //设置图片的大小和颜色通道
    TGAImage image(width, height, TGAImage::RGB);
    //读取模型
    model = new Model(model_path);
    
    //设置MVP矩阵
    lookat(cameraPos, centerPos, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection((cameraPos - centerPos).norm());
    //初始化zBuffer数组
    int *zBuffer = new int[width * height];
    for (int i = 0; i < width * height; i++)
    {
        zBuffer[i] = std::numeric_limits<int>::min();
    }

    GouraudShader shader;
    TextureShader tex_shader;
    NormalMappintShader norm_shader;
    NormalTangentShader tangent_shader;
    for (int idx = 0; idx < model->nfaces(); idx++)
    {
        Vec3f screen_coords[3];
        auto face_vert = model->face(idx);

        for (int j = 0; j < 3; j++)
        {
            auto uv = model->uv(idx, j);
            screen_coords[j] = tangent_shader.vertex(idx, j);
            //std::cout << uv.x <<' ' << uv.y << std::endl;
        }
        Shading(screen_coords, tangent_shader, image, zBuffer);
    }

    std::cout << "It's going to set the image." << std::endl;
    image.flip_vertically();
    image.flip_horizontally();
    image.write_tga_file(image_path);

    delete model;
    delete[] zBuffer;

    return 0;
}