#include "tgaimage.h"
#include <vector>
#include <cmath>
#include "model.h"
#include "geometry.h"
#include <algorithm>
#include <iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 0);
const TGAColor blue = TGAColor(0, 0, 255, 255);
Model *model = NULL;
const int width = 800;
const int height = 800;
const int depth = 255;

Vec3f cameraPos(5, 10, 20);
Vec3f centerPos(0, 0, 0);
Vec3f up(0, 1, 0);
Vec3f light_dir(0, 1, 0);
//计算重心坐标，返回1-u-v,u,v
Vec3f barycentric(const Vec3f *pts, const Vec2f p)
{
    Vec3f pos = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - p.x) ^ Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - p.y);

    if (std::abs(pos.z) < 1)
        return Vec3f(-1, 1, 1);

    return Vec3f(1.f - (pos.x + pos.y) / pos.z, pos.x / pos.z, pos.y / pos.z);
}

Vec3f world_to_screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

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
            auto bc = barycentric(pts, Vec2f(x, y));//获取重心坐标对应的(1-u-v), u ,v;

            auto alpha = bc.x, beta = bc.y , gamma = bc.z;
            
            Vec2f tex_pt(0, 0);
            if (alpha < 0 || beta < 0 || gamma < 0)//如果有一个是负数，说明点不在三角形内
                continue;

            float w_reciprocal = 1 / (alpha + beta + gamma);
            float z_interpolated = alpha * pts[0].z + beta * pts[1].z + gamma * pts[2].z;
            float x_interpolated = alpha * texs[0].x + beta * texs[1].x + gamma * texs[2].x;
            float y_interpolated = alpha * texs[0].y + beta * texs[1].y + gamma * texs[2].y;

            z_interpolated *= w_reciprocal;
            x_interpolated *= w_reciprocal;
            y_interpolated *= w_reciprocal;

            //std::cout << z_interpolated << ' ' << x_interpolated << ' ' << y_interpolated << std::endl;
            //if((int)(x + y * width) > height * width) std::cout << "out of range" << std::endl;
            //std::cout << int(x + y * imageWidth) << std::endl;
            int idx = x + y * width;
            if (idx < 0 || idx >= width * height) continue;
            if (zBuffer[idx] < z_interpolated)
            {
                
                TGAColor tex_color = model->diffuse().get(x_interpolated, y_interpolated);
                //std::cout << z_interpolated << ' ' << x_interpolated << ' ' << y_interpolated << std::endl;
                zBuffer[idx] = z_interpolated;
                //std::cout << "ok" << std::endl;
                TGAColor random_color = TGAColor(rand() % 255 , rand()%255, rand()%255);
                image.set(tmp_pt.x, tmp_pt.y, tex_color);
            }
        }
    }
}

void Gouraud_Shading(Vec3f *pts, Vec3f *vns, TGAImage &image, Vec3f look_dir, float *zBuffer)
{
    // std::cout << "It's going to paint the image" << std::endl;
    int min_x = std::min({pts[0].x, pts[1].x, pts[2].x});
    int max_x = std::max({pts[0].x, pts[1].x, pts[2].x});
    int min_y = std::min({pts[0].y, pts[1].y, pts[2].y});
    int max_y = std::max({pts[0].y, pts[1].y, pts[2].y});
    std::cout << min_x << ' ' << max_x << ' ' << min_y << ' ' << max_y << std::endl;

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            // std::cout << x << ' ' << y << std::endl;
            auto barycentic_pos = barycentric(pts, Vec2f(x, y));

            if (barycentic_pos.x < -.01 or barycentic_pos.y < -.01 or barycentic_pos.z < -.01)
                continue;

            float z = 0;
            float intensity = 0;
            for (int i = 0; i < 3; i++)
            {
                z += barycentic_pos[i] * pts[i].z;
                intensity += (look_dir * vns[i].normalize()) * barycentic_pos[i];
            }
            // std::cout << (int)(x + y * width) << std::endl;
            if (zBuffer[(int)(x + y * width)] < z)
            {
                zBuffer[(int)(x + y * width)] = z;
                // std::cout << intensity << std::endl;
                image.set(x, y, TGAColor(255 * intensity, 255 * intensity, 255 * intensity));
            }
        }
    }
}

/**
 * params：Vec3f v
 * 考虑到涉及三维空间中的变换，需要一个函数将v变为对应的齐次坐标
 */
Matrix convert_to_homo(Vec3f v)
{
    Matrix res(4, 1);
    res[0][0] = v.x;
    res[1][0] = v.y;
    res[2][0] = v.z;
    res[3][0] = 1;
    return res;
}

Matrix viewport_trans()
{
    return Matrix::identity(4);
}

Matrix model_trans()
{
    return Matrix::identity(4);
}

Matrix proj_to_ortho()
{
    Matrix projection = Matrix::identity(4);
    projection[3][2] = -1.0f / (cameraPos - centerPos).norm();
    return projection;
}

Matrix projection_division(Matrix m)
{
    // std::cout << m[3][0] << std::endl;
    m[0][0] = m[0][0] / m[3][0];
    m[1][0] = m[1][0] / m[3][0];
    m[2][0] = m[2][0] / m[3][0];
    m[3][0] = 1.f;
    return m;
}

Matrix viewportMatrix(int x, int y, int w, int h)
{
    Matrix m = Matrix::identity(4);
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}

Vec3f homo_to_vert(Matrix m)
{
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

Matrix lookAt(Vec3f eye, Vec3f center, Vec3f up)
{
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();

    Matrix res = Matrix::identity(4);
    Matrix rotation = Matrix::identity(4);
    Matrix translation = Matrix::identity(4);

    for (int i = 0; i < 3; i++)
    {
        rotation[0][i] = x[i];
        rotation[1][i] = y[i];
        rotation[2][i] = z[i];
        translation[i][3] = -center[i];
    }
    res = rotation * translation;

    return res;
}

int main(int argc, char **argv)
{
    TGAImage image(width, height,TGAImage::RGB);
    model = new Model("../obj/floor.obj");

    //进行MVP转换
    //Matrix _model = model_trans();
    Matrix _model = lookAt(cameraPos, centerPos, up);
    Matrix _projection = proj_to_ortho();
    Matrix _viewport = viewportMatrix(width / 8 , height / 8, width * 3 / 4, height * 3 / 4);

    float *zBuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
    {
        zBuffer[i] = std::numeric_limits<float>::min();
    }

    for (int idx = 0; idx < model->nfaces(); idx++) 
    {
        std::vector<int> face_verts = model->face(idx);
        std::vector<int> tex_verts = model->fuvs(idx);
        std::vector<int> norm_verts = model->fnorms(idx);
        Vec3f world_coords[3];
        Vec3f screen_coords[3];
        Vec2i tex_coords[3];
        Vec3f norm_coords[3];

        for (int j = 0; j < 3; j++)
        {
            world_coords[j] = model->vert(face_verts[j]);
            screen_coords[j] = homo_to_vert(_viewport *((_projection*_model* convert_to_homo(world_coords[j])))) ;
            tex_coords[j] = model->uv(tex_verts[j]);
            norm_coords[j] = model->norm(norm_verts[j]);
        }

        //计算这个面的法线
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        texture_shading(screen_coords, tex_coords, image, light_dir, zBuffer);
    }

    std::cout << "It's going to set the image." << std::endl;
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("../output_floor_diffuse.tga");

    delete model;
    delete[] zBuffer;


    return 0;
}
