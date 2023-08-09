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

Vec3f cameraPos(0, 0, 5);
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
        //varying_uv.set_col(nthvert, model->uv(iface, nthvert));
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

/*
void texture_shading(Vec3f *pts, Vec2i *texs, TGAImage &image, Vec3f look_dir, float *zBuffer)
{
    // 首先获取对应的Bounding Box
    float min_x = std::min({pts[0].x, pts[1].x, pts[2].x});
    float max_x = std::max({pts[0].x, pts[1].x, pts[2].x});
    float min_y = std::min({pts[0].y, pts[1].y, pts[2].y});
    float max_y = std::max({pts[0].y, pts[1].y, pts[2].y});

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            Vec3f tmp_pt(x, y, 0);
            auto bc = barycentric(pts, Vec2f(x, y)); // 获取重心坐标对应的(1-u-v), u ,v;

            auto alpha = bc.x, beta = bc.y, gamma = bc.z;

            if (alpha < 0 || beta < 0 || gamma < 0) // 如果有一个是负数，说明点不在三角形内
                continue;

            //插值计算
            float z_interpolated = interpolate(pts[0].z, pts[1].z, pts[2].z, alpha, beta, gamma);
            float x_interpolated = interpolate(texs[0].x, texs[1].x, texs[2].x, alpha, beta, gamma);
            float y_interpolated = interpolate(texs[0].y, texs[1].y, texs[2].y, alpha, beta, gamma);

            // 判断是否存在越界情况
            int idx = x + (y)*width;
            if (idx < 0 || idx >= width * height)
                continue;
            if (zBuffer[idx] < z_interpolated)
            {
                zBuffer[idx] = z_interpolated;
                image.set(tmp_pt.x, tmp_pt.y, model->diffuse(x_interpolated, y_interpolated));
            }
        }
    }
}
*/

int main(int argc, char **argv)
{
    //设置图片的大小和颜色通道
    TGAImage image(width, height, TGAImage::RGB);
    //读取模型
    model = new Model(model_path);
    
    //设置MVP矩阵
    lookat(cameraPos, centerPos, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection(-1.f / (cameraPos - centerPos). norm());
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

        for (int j = 0; j < 3; j++)
        {
            screen_coords[j] = tex_shader.vertex(idx, j);
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
