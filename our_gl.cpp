
#include <cmath>
#include <limits>  
#include <cstdlib>
#include "our_gl.h"


Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void viewport(int x, int y, int w, int h)
{
    Viewport = Matrix::identity(4);
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 255 / 2.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 255 / 2.f;
}

void projection(float coeff)
{
    Projection = Matrix::identity(4);
    Projection[3][2] = coeff;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up)
{
    Vec3f z = (eye - center).normalize();
    Vec3f x = (z ^ up).normalize();
    Vec3f y = (x ^ z).normalize();
    ModelView = Matrix::identity(4);

    for (int i = 0; i < 3; i++)
    {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
}

Vec3f barycentric(const Vec3f *pts, const Vec2f p)
{
    Vec3f pos = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - p.x) ^ Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - p.y);

    if (std::abs(pos.z) < 1)
        return Vec3f(-1, 1, 1);

    return Vec3f(1.f - (pos.x + pos.y) / pos.z, pos.x / pos.z, pos.y / pos.z);
}

float interpolate(float p1, float p2, float p3, float alpha, float beta, float gamma)
{
    float w_reciprocal = 1.0 / (alpha + beta + gamma);
    return (alpha * p1 + beta * p2 + gamma * p3) * w_reciprocal;
}

Matrix convert_to_homo(Vec3f v)
{
    Matrix res(4, 1);
    res[0][0] = v.x;
    res[1][0] = v.y;
    res[2][0] = v.z;
    res[3][0] = 1;
    return res;
}

Vec3f homo_to_vert(Matrix m)
{
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

void Shading(Vec3f *pts, IShader &shader, TGAImage &image, int *zbuffer)
{
    int min_x = std::min(pts[0].x, std::min(pts[1].x, pts[2].x));
    int max_x = std::max(pts[0].x, std::max(pts[1].x, pts[2].x));
    int min_y = std::min(pts[0].y, std::min(pts[1].y, pts[2].y));
    int max_y = std::max(pts[0].y, std::max(pts[1].y, pts[2].y));

    TGAColor color(255, 255, 255);

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            // std::cout << x << ' ' << y << std::endl;
            auto bc = barycentric(pts, Vec2f(x, y)); // 获取重心坐标对应的(1-u-v), u ,v;

            auto alpha = bc.x, beta = bc.y, gamma = bc.z;

            if (alpha < 0 || beta < 0 || gamma < 0) // 如果有一个是负数，说明点不在三角形内
                continue;
            

            float w_reciprocal = 1 / (alpha + beta + gamma);
            float z_interpolated = alpha * pts[0].z + beta * pts[1].z + gamma * pts[2].z;
            z_interpolated *= w_reciprocal;
            

            int idx = x + y * image.get_width();
            if(idx < 0) continue;

            if(zbuffer[idx] > z_interpolated) continue;
            bool discard = shader.fragment(bc * w_reciprocal, color);
            if(!discard) 
                zbuffer[idx] = z_interpolated;
                image.set(x, y, color);
            }
        }
    }
