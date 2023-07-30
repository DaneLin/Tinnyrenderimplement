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
const int width = 1024;
const int height = 1024;
const int depth = 255;

Vec3f cameraPos(1, 1, 3);
Vec3f centerPos(0 ,0, 0);
Vec3f up(0, 1, 0);

Vec3f barycentric(const Vec3f* pts, const Vec2f p)
{
    Vec3f pos = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - p.x ) ^ Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y -p.y );

    if (std::abs(pos.z) < 1) return Vec3f(-1, 1, 1);

    return Vec3f(1.f - (pos.x + pos.y) / (float)pos.z, (float)pos.x / pos.z, (float)pos.y / pos.z);

}

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    // step size for 0.1
    //  for (float t = 0.; t < 1.; t += .01)
    //  {
    //      int x = x0 + (x1 - x0) * t;
    //      int y = y0 + (y1 - y0) * t;
    //      image.set(x, y, color);
    //  }

    // second attempt
    //  for (int x = x0; x <= x1; x++)
    //  {
    //      float t = (x - x0) / (float)(x1 - x0);
    //      int y = y0 * (1.0f - t) + y1 * t;
    //      image.set(x, y, color);
    //  }

    // third attempt
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        // 交换x，y,并且设置steep为true
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    // 保证从x0到x1是从小到大
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x = x0; x <= x1; x++)
    {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1.0f - t) + y1 * t;
        if (steep)
        {
            image.set(y, x, color);
        }
        else
        {
            image.set(x, y, color);
        }
    }

    // fourth attempt
    //  bool steep  = false;
    //  if (std::abs(x0 - x1) < std::abs(y0 - y1))
    //  {
    //      std::swap(x0, y0);
    //      std::swap(x1, y1);
    //      steep = true;
    //  }
    //  if (x0 > x1)
    //  {
    //      std::swap(x0, x1);
    //      std::swap(y0, y1);
    //  }

    // int dx = x1 - x0;
    // int dy = y1- y0;
    // float derror = std::abs(dy / (float)(dx));
    // float error = 0;
    // int y = y0;
    // for (int x = x0; x <= x1 ; x++)
    // {
    //     if (steep)
    //     {
    //         image.set(y, x, color);
    //     }else {
    //         image.set(x, y, color);
    //     }
    //     error += derror;
    //     if (error > .5f)
    //     {
    //         y += (y1 > y0?1:-1);
    //         error -= 1;
    //     }
    // }

    // //fifth attempt for timing optimization
    // bool steep = false;
    // if (std::abs(x0 - x1) < std::abs(y0 - y1))
    // {
    //     std::swap(x0, y0);
    //     std::swap(x1, y1);
    //     steep = true;
    // }
    // if (x0 > x1) {
    //     std::swap(x0, x1);
    //     std::swap(y0, y1);
    // }

    // int dx = x1 - x0;
    // int dy = y1 - y0;
    // int derror2 = std::abs(dy) * 2;
    // int error2 = 0;
    // int y = y0;

    // for (int x = x0; x <= x1; x++)
    // {
    //     if (steep)
    //     {
    //         image.set(y, x, color);
    //     }
    //     else {
    //         image.set(x, y, color);
    //     }
    //     error2 += derror2;
    //     if (error2 > dx)
    //     {
    //         y += (y1 > y0?1 : -1);
    //         error2 -= dx*2;
    //     }
    // }
}

//传统检测点方法
void triangle(Vec2i vert[], TGAImage &image, TGAColor color)
{
    /**
     * step1、找到三角形的边界
     * step2、找到三角形的左右边界，连续从左向右画线
     */
    // 首先将顶点按照y进行排序，这里从小到大排序
    std::sort(vert, vert + 3, [](Vec2i a, Vec2i b)
              { return a.y > b.y; });

    Vec2i top_point, middle_point, bottom_point;
    top_point = vert[0];
    middle_point = vert[1];
    bottom_point = vert[2];

    // second attempt
    //  for (int x = x0; x <= x1; x++)
    //  {
    //      float t = (x - x0) / (float)(x1 - x0);
    //      int y = y0 * (1.0f - t) + y1 * t;
    //      image.set(x, y, color);
    //  }
    //一行一行绘制
    //以中间点作为分割点，将三角形分为上下两个部分
    //先将三角形的下半部分绘制
    int tot_height = top_point.y - bottom_point.y;
    int seg_height = middle_point.y - bottom_point.y + 1;
    for (int y = bottom_point.y; y <= middle_point.y; y++)
    {
        float b_to_m = (float)(y - bottom_point.y) / seg_height;
        float b_to_t = (float)(y - bottom_point.y) / tot_height;

        Vec2i bmv = bottom_point + (middle_point - bottom_point) * b_to_m;
        Vec2i btv = bottom_point + (top_point - bottom_point) * b_to_t;
        if (bmv.x > btv.x) std::swap(bmv.x, btv.x);

        for (int x = bmv.x; x <= btv.x; x++)
        {
            image.set(x, y, color);
        }
    }
    //绘制三角形的上半部分
    seg_height = top_point.y - middle_point.y + 1;
    for (int y = middle_point.y; y <= top_point.y; y++)
    {
        float m_t_t = (float)(y - middle_point.y) / seg_height;
        float b_t_t = (float)(y - bottom_point.y) / tot_height;

        Vec2i mtv = middle_point + (top_point - middle_point) * m_t_t;
        Vec2i btv = bottom_point + (top_point - bottom_point) * b_t_t;
        
        if(mtv.x > btv.x) std::swap(mtv.x, btv.x);
        for (int x = mtv.x; x <= btv.x; x++)
        {
            image.set(x, y, color);
        }
    }
}

//使用重点坐标和包围盒方式优化的检测方式
/*

void triangle_with_barycentric(const Vec2i *pts, TGAImage &image, TGAColor color)
{
    int min_x = std::min({pts[0].x, pts[1].x, pts[2].x});
    int max_x = std::max({pts[0].x, pts[1].x, pts[2].x});
    int min_y = std::min({pts[0].y, pts[1].y, pts[2].y});
    int max_y = std::max({pts[0].y, pts[1].y, pts[2].y});

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            Vec3f barycentric_pos = barycentric(pts, Vec2i(x, y));

            if (barycentric_pos.x < 0 || barycentric_pos.y < 0 || barycentric_pos.z < 0) continue;
            image.set(x, y, color);
        }
    }

}
*/

//2D情况下的光栅化
void rasterizer(Vec2i p0,  Vec2i p1, TGAImage& image, const TGAColor &color, int *ybuffer)
{
    if (p0.x > p1.x) {
        std::swap(p0, p1);
    }

    float tot_len = (p1.x - p0.x) + 1;
    for (int x = p0.x; x <= p1.x; x++)
    {
        float t = (x - p0.x) / tot_len;
        int y = p0.y * (1. - t) + p1.y * t;
        if (ybuffer[x] < y) {
            ybuffer[x] = y;
            for (int j = 0; j < 16; j++)
                image.set(x, j, color);
        }
    }
}

/*

void rasterizer3D(Vec3f* pts, TGAImage &image, int *zbuffer)
{
    //传进来三角形的三个点
    float min_x = std::min({pts[0].x, pts[1].x, pts[2].x});
    float max_x = std::max({pts[0].x, pts[1].x, pts[2].x});
    float min_y = std::min({pts[0].y, pts[1].y, pts[2].y});
    float max_y = std::max({pts[0].y, pts[1].y, pts[2].y});

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            Vec3f barycentric_pos = barycentric(pts, Vec2i(x, y));

            if (barycentric_pos.x < 0 || barycentric_pos.y < 0 || barycentric_pos.z < 0) continue;
            Vec3f p(x, y, 0);
            
            for (int i = 0; i < 3; i++)
            {
                Vec3f tmp = barycentric_pos ^ pts[i];
                p = p + tmp;
            }
            if (p.z > zbuffer[(int)(x + y * width)])
            {
                zbuffer[(int)(x + y * width)] = p.z;
                TGAColor color = image.get((int)p.x * image.get_width(), (int)p.y * image.get_height());
                image.set(x, y, color);
            }
        }
    }

}
*/

Vec3f world_to_screen(Vec3f v)
{
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

void texture_shading(Vec3f *pts, Vec2i *texs, TGAImage &image, TGAImage &texture, Vec3f look_dir, float *zBuffer)
{
    //首先获取对应的Bounding Box
    float min_x = std::min({pts[0].x, pts[1].x, pts[2].x});
    float max_x = std::max({pts[0].x, pts[1].x, pts[2].x});
    float min_y = std::min({pts[0].y, pts[1].y, pts[2].y});
    float max_y = std::max({pts[0].y, pts[1].y, pts[2].y});
     int w = texture.get_width();
    int h = texture.get_height();
    // std::cout << w << ' ' << h << std::endl;
    // for (int i = 0; i < 3; i++)
    // {
    //     std::cout << texs[i].x  << ' ' << texs[i].y  << std::endl;
    // }

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            Vec3f tmp_pt(x, y, 0);
            Vec3f barycentric_pos = barycentric(pts, Vec2f(x, y));
            Vec2f tex_pt(0, 0);
            if (barycentric_pos.x < -.01 || barycentric_pos.y < -.01 || barycentric_pos.z < -.01) continue;
        
            //std::cout << w <<' ' << h << std::endl;
            for (int i = 0; i < 3; i++) 
            {
                tmp_pt.z += barycentric_pos[i] * pts[i].z;
                tex_pt.x += texs[i].x * barycentric_pos[i];
                tex_pt.y += texs[i].y * barycentric_pos[i];
                //std::cout << texs[i].x << ' ' << texs[i].y << std::endl;
            }
           //std::cout << std::endl;
            //std::cout << tex_pt.x << ' ' << tex_pt.y << ' ' << w << ' ' << h<< std::endl;
            TGAColor tex_color = texture.get(tex_pt.x, tex_pt.y);
            //std::cout << tex_color.bgra[2] << ' ' << tex_color.bgra[1] << ' ' << tex_color.bgra[0] << ' ' << std::endl;

            if (zBuffer[(int)(x + y * width)] < tmp_pt.z)
            {
                zBuffer[(int)(x + y * width)] = tmp_pt.z;
                image.set(tmp_pt.x, tmp_pt.y, tex_color);
            }
        }
    }

}


void Gouraud_Shading(Vec3f *pts, Vec3f *vns, TGAImage &image, Vec3f look_dir, float *zBuffer)
{
    int min_x = std::min({pts[0].x, pts[1].x, pts[2].x});
    int max_x = std::max({pts[0].x, pts[1].x, pts[2].x});
    int min_y = std::min({pts[0].y, pts[1].y, pts[2].y});
    int max_y = std::max({pts[0].y, pts[1].y, pts[2].y});

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            auto barycentic_pos = barycentric(pts,Vec2f(x, y));

            if (barycentic_pos.x < -.01 or barycentic_pos.y < -.01 or barycentic_pos.z < -.01) continue;

            float z = 0;
            float intensity = 0;
            for (int i = 0; i < 3; i++)
            {
                z += barycentic_pos[i] * pts[i].z;
                intensity += (look_dir * vns[i].normalize()) * barycentic_pos[i]  ;
            }

            if (zBuffer[(int)(x + y * width)] < z)
            {
                zBuffer[(int)(x + y * width)] = z;
                //std::cout << intensity << std::endl;
                image.set(x, y, TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255 * intensity));
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
    projection[3][2] = -1.0f / cameraPos.z;
    return projection;
}

Matrix projection_division(Matrix m)
{
    m[0][0] = m[0][0] / m[3][0];
    m[1][0] = m[1][0] / m[3][0];
    m[2][0] = m[2][0] / m[3][0];
    m[3][0] = 1.f;
    return m;
}

Matrix viewportMatrix(int x, int y, int w, int h) {
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
    return Vec3f(m[0][0], m[1][0], m[2][0]);
}

Matrix lookAt(Vec3f eye, Vec3f center, Vec3f up)
{
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();

    Matrix res = Matrix::identity(4);

    for (int i = 0; i < 3; i++)
    {
        res[0][i] = x[i];
        res[1][i] = y[i];
        res[2][i] = z[i];
        res[i][3] = -center[i];
    }

    return res;
}



int main(int argc, char **argv)
{
   
   TGAImage image = TGAImage(width, height, TGAImage::RGB);
   float *zBuffer = new float[width * height];
   
   for (int i = 0; i < width * height; i++) 
   {
        zBuffer[i] = std::numeric_limits<float>::min();
   }

   TGAImage texture = TGAImage(width, height, TGAImage::RGB);
   texture.read_tga_file("../obj/african_head_diffuse.tga");//读取纹理
   texture.flip_vertically();
    model = new Model("../obj/african_head.obj");//读取模型信息
    Matrix model_ = lookAt(cameraPos, centerPos, up);
    Matrix view_ = viewport_trans();
    Matrix projection_ = proj_to_ortho();
    Matrix viewport_ = viewportMatrix(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

   Vec3f light_dir(0, 0, 1);//设置光线
   light_dir.normalize();

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
            Vec3f vert_pos = model->vert(face_verts[j]);
            world_coords[j] = vert_pos;
            screen_coords[j] = homo_to_vert(viewport_*projection_division(projection_*view_*model_*convert_to_homo(world_coords[j])));
            tex_coords[j] = model->uv(tex_verts[j]);
            norm_coords[j] = model->norm(norm_verts[j]);
            //std::cout << tex_verts[j] << ' ' << model->uv(tex_verts[j]).x << std::endl;
            //std::cout << tex_coords[j].x << ' ' << tex_coords[j].y << std::endl;
        }

        // Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        // n.normalize();
        // float intensity = n * light_dir;
        // if (intensity > 0)
        texture_shading(screen_coords, tex_coords, image, texture, light_dir, zBuffer);
        //Gouraud_Shading(screen_coords, norm_coords, image, light_dir, zBuffer);

   }
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("../output_lesson33.tga");

    { // dump z-buffer (debugging purposes only)
        TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
        for (int i=0; i<width; i++) {
            for (int j=0; j<height; j++) {
                zbimage.set(i, j, TGAColor(zBuffer[i+j*width]));
            }
        }
        zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        zbimage.write_tga_file("../zbuffer.tga");
    }
    delete model;
    delete []zBuffer;

    return 0;
}
