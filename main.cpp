#include "tgaimage.h"
#include <vector>
#include <cmath>
#include "model.h"
#include "geometry.h"
#include <algorithm>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 0);
Model *model = NULL;
const int width = 800;
const int height = 800;

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

    TGAImage image(200, 200, TGAImage::RGB);
    Vec2i t0[3] = {Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80)};
    Vec2i t1[3] = {Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180)};
    Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};

    triangle(t0, image, red);
    triangle(t1, image, white);
    triangle(t2, image, green);

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output3.tga");

    return 0;
}
