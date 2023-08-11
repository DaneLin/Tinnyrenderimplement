#include "tgaimage.h"
#include <vector>
#include <cmath>
#include "model.h"
// #include "geometry.h"
#include "geometry.h"
#include "our_gl.h"
#include <algorithm>
#include <iostream>

const int width = 800;
const int height = 800;
Model *model = NULL;
const int depth = 255;
const char *model_path = "../obj/diablo3/diablo3_pose.obj";
const char *image_path = "../output/output_african_head_Depth.tga";
float *shadowBuffer;

Vec3f cameraPos(0, 10, 5);
Vec3f centerPos(0, 0, 0);
Vec3f up(0, 1, 0);
Vec3f light_dir(-1, 1, 1);

std::vector<float> calc_diff(const std::vector<float> &a, const std::vector<float> &b)
{
    int size = a.size();
    std::vector<float> res(size);
    for (int i = 0; i < size; i++)
    {
        res[i] = a[i] - b[i];
    }
    return res;
}

struct GouraudShader : public IShader
{
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

struct TextureShader : public IShader
{
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
        for (int i = 0; i < 3; i++)
            tmp[i][0] = bar[i];
        float intensity = varying_intensity * bar;
        Matrix uv = varying_uv * tmp;
        color = model->diffuse(uv[0][0], uv[1][0]) * intensity;
        return false;
    }
};

// interpret RGB values as xyz direction
struct NormalMappintShader : public IShader
{
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
        for (int i = 0; i < 3; i++)
            tmp[i][0] = bar[i];
        Matrix uv = varying_uv * tmp;
        varying_nm = model->normal(uv[0][0], uv[1][0]).normalize();
        auto intensity = varying_nm * light_dir;
        color = model->diffuse(uv[0][0], uv[1][0]) * intensity;
        return false;
    }
};

// 使用法线空间法线贴图
struct NormalTangentShader : public IShader
{
    Matrix varying_uv = Matrix(2, 3);  // 存储顶点UV坐标，顶点着色器写入，片元着色器读取
    Matrix varying_tri = Matrix(3, 3); // 存储顶点裁剪空间坐标
    Matrix varying_nrm = Matrix(3, 3); // 存储顶点法线
    Matrix ndc_tri = Matrix(3, 3);     // 存储顶点归一化的设备坐标，用于切线空间计算

    virtual Vec3f vertex(int iface, int nthvert)
    {
        // 读取顶点uv坐标
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        // 将顶点法线转换到view space
        varying_nrm.set_col(nthvert, homo_to_vert((Projection * ModelView).inverse().transpose() * convert_to_homo(model->normal(iface, nthvert))));
        // 获取裁剪空间坐标
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));
        varying_tri.set_col(nthvert, gl_vert);
        ndc_tri.set_col(nthvert, gl_vert);

        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        // 插值顶点法线
        Vec3f bn = (varying_nrm * bar).normalize();
        // 插值顶点uv
        Vec3f uv = varying_uv * bar;

        // // 计算切线空间到观察空间的变换矩阵
        // Matrix A(3, 3);
        // A[0] = calc_diff(ndc_tri[1], ndc_tri[0]);
        // A[1] = calc_diff(ndc_tri[2], ndc_tri[0]);
        // for (int i = 0; i < 3; i++)
        // {
        //     A[2][i] = bn.raw[i];
        // }
        // Matrix AI = A.inverse();

        // // 利用UV计算切线空间基
        // Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        // Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        // Matrix B(3);
        // B.set_col(0, i.normalize());
        // B.set_col(1, j.normalize());
        // B.set_col(2, bn);

        // Vec3f n = (B * model->normal(uv.x, uv.y)).normalize();

        // 光照计算
        float diff = std::max(0.f, bn * light_dir);
        color = model->diffuse(uv.x, uv.y) * diff;
        return false;
    }
};

struct Shader : public IShader
{
    Matrix varying_uv = Matrix(2, 3);
    Matrix uniform_M = Matrix(4, 4);   // Projection * ModelView
    Matrix uniform_MIT = Matrix(4, 4); // 转换矩阵的逆转置
    Matrix uniform_Mshadow = Matrix(4, 4);
    Matrix varying_tri = Matrix(3, 3);

    Shader(Matrix M, Matrix MIT, Matrix MS) : uniform_M(M), uniform_MIT(MIT), uniform_Mshadow(MS), varying_tri(), varying_uv() {}

    virtual Vec3f vertex(int iface, int nthvert)
    {
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_tri.set_col(nthvert, gl_vert);
        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        Vec3f sb_p = homo_to_vert(uniform_Mshadow * convert_to_homo(varying_tri * bar));
        int idx = (int)sb_p.x + (int)sb_p.y * width;
        float shadow = .3f + .7f * (shadowBuffer[idx] < sb_p.z +43.34);//magic coeff to avoid z-fighting
        Vec3f uv = varying_uv * bar;
        Vec3f n = homo_to_vert(uniform_MIT * convert_to_homo(model->normal(uv.x, uv.y))).normalize();
        Vec3f l = homo_to_vert(uniform_M * convert_to_homo(light_dir)).normalize();
        Vec3f r = (n * (n * l * 2.f) - l).normalize();
        float spec = std::pow(std::max(r.z, 0.0f), model->specular(uv.x, uv.y));
        float diff = std::max(0.f, n * l);
        color = model->diffuse(uv.x, uv.y);

        for (int i = 0; i < 3; i++)
        {
            color[i] = std::min<float>(20 + color[i] * shadow * (1.2 * diff + .6f * spec), 255);
        }

        return false;
    }
};

struct DepthShader : public IShader
{
    Matrix varying_tri = Matrix(3, 3);

    virtual Vec3f vertex(int iface, int nthvert)
    {
        Vec3f gl_vert = model->vert(iface, nthvert);
        gl_vert = homo_to_vert(Viewport * Projection * ModelView * convert_to_homo(gl_vert));

        varying_tri.set_col(nthvert, gl_vert);
        return gl_vert;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        Vec3f p = varying_tri * bar;
        color = TGAColor(255, 255, 255) * (p.z / depth);
        return false;
    }
};

int main(int argc, char **argv)
{
    // if (argc < 2) {
    //     std::cerr << "Please use obj file" << std::endl;
    //     return 1;
    // }

    // 设置图片的大小和颜色通道
    TGAImage image(width, height, TGAImage::RGB);
    
    // 读取模型
    model = new Model(model_path);

    // 设置MVP矩阵
    lookat(cameraPos, centerPos, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection((cameraPos - centerPos).norm());
    light_dir = homo_to_vert(Projection * ModelView * convert_to_homo(light_dir)).normalize(); // 转换到相机空间

    // 初始化zBuffer和shadowbuffer数组
    float *zBuffer = new float[width * height];
    shadowBuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
    {
        zBuffer[i] = shadowBuffer[i] = std::numeric_limits<float>::min();
    }

    {//设置shadowbuffer
        TGAImage depth(width, height, TGAImage::RGB);
        lookat(light_dir, centerPos, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(0);

        DepthShader depthShader;
        Vec3f screen_coords[3];
        for (int i = 0; i < model->nfaces(); i++)
        {
            for (int j = 0; j < 3; j++)
            {
                screen_coords[j] = depthShader.vertex(i, j);
            }
            Shading(screen_coords, depthShader, depth, shadowBuffer);
        }
        depth.flip_vertically();
        depth.flip_horizontally();
        std::cout << "It's going to paint depth" << std::endl;
        depth.write_tga_file("depth.tga");
    }

    Matrix M = Viewport * Projection * ModelView;

    {
        TGAImage frame(width, height, TGAImage::RGB);
        lookat(cameraPos, centerPos, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (cameraPos - centerPos).norm());

        Shader shader(ModelView, (Projection * ModelView).inverse().transpose(), M * (Viewport * Projection * ModelView).inverse());
        Vec3f screen_coords[3];
        for (int i = 0; i < model->nfaces(); i++)
        {
            for (int j = 0; j < 3; j++)
            {
                screen_coords[j] = shader.vertex(i, j);
            }
            Shading(screen_coords, shader, frame, zBuffer);
        }
        frame.flip_vertically();
        frame.flip_horizontally();
        std::cout << "It's going to paint frame" << std::endl;
        frame.write_tga_file("framebuffer.tga");
    }

    delete model;
    delete[] zBuffer;

    return 0;
}