#include "tgaimage.h"
#include <vector>
#include <cmath>
#include "model.h"
#include "geometry.h"
#include <algorithm>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 0);
const TGAColor blue = TGAColor(0, 0, 255, 255);
Model *model = NULL;
const int width = 1024;
const int height = 1024;

Vec3f barycentric(const Vec3f* pts, const Vec2i p)
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
                TGAColor color = image.get((int)p.x, (int)p.y);
                image.set(x, y, color);
            }
        }
    }

}

int main(int argc, char **argv)
{
    // TGAImage image(800, 800, TGAImage::RGB);
    // //image.set(52, 41, red);
    // line(13, 20, 80, 40, image, white);
    // line(20, 13, 40, 80, image,red);
    // line(80, 40, 13, 20,image,red);
    // image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    // image.write_tga_file("output.tga");

    // 加载模型
    /*
    model = new Model("D:\\Github\\TinyRendererImplement\\obj/african_head.obj");
    // 设置Image
    TGAImage image(width, height, TGAImage::RGB);
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;
            line(x0, y0, x1, y1, image, white);
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output1.tga");
    delete model;
    */

    /*
    
    TGAImage image(200, 200, TGAImage::RGB);
    Vec2i t0[3] = {Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80)};
    Vec2i t1[3] = {Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180)};
    Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};

    //使用最原始的检测方式
   
    triangle(t0, image, red);
    triangle(t1, image, white);
    triangle(t2, image, green);

    //使用优化过的重心坐标方式
    // triangle_with_barycentric(t0, image, red);
    // triangle_with_barycentric(t1, image, white);
    // triangle_with_barycentric(t2, image, green);
    */
    
    // model = new Model("D:\\Github\\TinyRendererImplement\\obj/african_head.obj");
    // TGAImage image(width, height, TGAImage::RGB);

    // Vec3f light_dir(0, 0, -1);
    
    // for (int i = 0; i < model->nfaces(); i++)
    // {
    //     std::vector<int> face = model->face(i);
    //     Vec2i screen_coords[3];
    //     Vec3f world_coords[3];
    //     for (int j = 0; j < 3; j++)
    //     {
    //         Vec3f v = model->vert(face[j]);
    //         screen_coords[j] = Vec2i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2.);
    //         world_coords[j] = v;
    //     }
    //     Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
    //     n.normalize();
    //     float intensity = n * light_dir;
    //     triangle_with_barycentric(screen_coords, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
    // }

    // image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    // image.write_tga_file("output_flat_light.tga");

    //Lesson3 
    // two dimension situation
    /*
    TGAImage render (width, 16, TGAImage::RGB);
    int ybuffer[width];
    for (int i = 0; i < width; i++)
    {
        ybuffer[i] = std::numeric_limits<int>::min();
    }

    rasterizer(Vec2i(20, 34), Vec2i(744, 400), render, red, ybuffer);
    rasterizer(Vec2i(120, 434), Vec2i(444, 400), render, green, ybuffer);
    rasterizer(Vec2i(330, 463), Vec2i(594, 200), render, blue, ybuffer);

    render.flip_vertically();
    render.write_tga_file("renderer.tga");
    */

    // 3D
    TGAImage image = TGAImage(width, height, TGAImage::RGB);
    int *zBuffer = new int[width * height];

    for (int i = 0 ; i < width * height; i++)
    {
        zBuffer[i] = std::numeric_limits<int>::min();
    }

    model = new Model("D:\\Github\\TinyRendererImplement\\obj/african_head.obj");
    image.read_tga_file("D:\\Github\\TinyRendererImplement\\obj/african_head.obj");
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f verts[3];
        for (int j = 0; j < 3; j++)
        {
            Vec3f v0 = model->vert(face[j]);
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            verts[j].x = x0;
            verts[j].y = y0;
            verts[j].z = v0.z;
        }
        rasterizer3D(verts, image,zBuffer);
    }  

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output_lesson3.tga");
    delete model;


    return 0;
}
