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
const char *image_path = "../output/output_african_head_texture.tga";

Vec3f cameraPos(2, 2, 5);
Vec3f centerPos(0, 0, 0);
Vec3f up(0, 1, 0);
Vec3f light_dir = Vec3f(0, -1, 1).normalize();

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
        //std::cout << uv[0][0] << ' ' << uv[1][0] << ' ';
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
    projection(-1.f / (cameraPos - centerPos).norm());
    //初始化zBuffer数组
    int *zBuffer = new int[width * height];
    for (int i = 0; i < width * height; i++)
    {
        zBuffer[i] = std::numeric_limits<int>::min();
    }

    GouraudShader shader;
    TextureShader tex_shader;
    for (int idx = 0; idx < model->nfaces(); idx++)
    {
        Vec3f screen_coords[3];
        auto face_vert = model->face(idx);

        for (int j = 0; j < 3; j++)
        {
            auto uv = model->uv(idx, j);
            screen_coords[j] = tex_shader.vertex(idx, j);
            //std::cout << uv.x <<' ' << uv.y << std::endl;
        }
        Shading(screen_coords, tex_shader, image, zBuffer);
    }

    std::cout << "It's going to set the image." << std::endl;
    image.flip_vertically();
    image.write_tga_file(image_path);

    delete model;
    delete[] zBuffer;

    return 0;
}