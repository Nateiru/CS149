#ifndef __REF_RENDERER_H__
#define __REF_RENDERER_H__

#include "circleRenderer.h"


class RefRenderer : public CircleRenderer {

private:

    Image* image;        // 最终渲染图像
    SceneName sceneName; // 图像的名称是一个枚举，为了渲染不同的图像

    /* 根据 sceneName 通过 sceneLoader 加载数据存储在下面字段中*/
    int numCircles;      // 
    float* position;     // 圆心位置 [x0, y0, z0], [x1, y1, z1] 渲染二维 z 的作用在这里和透明度 alpha 有关
    float* velocity;     // 每帧渲染时移动的速度  [vx0, vy0, vz0] 
    float* color;        // 圆的颜色 [r0, g0, b0], [r1, g1, b1]
    float* radius;       // 圆的半径 []

public:

    RefRenderer();
    virtual ~RefRenderer();

    const Image* getImage();                     // 获得最终渲染图像

    void setup();                                // 配置, 比如 cuda 前的 cudaMalloc

    void loadScene(SceneName name);              // 根据 SceneName 加载图像数据

    void allocOutputImage(int width, int height);// 给 Image* image 手动分配内存

    void clearImage();                           // 清空 image 但是不释放内存

    void advanceAnimation();                     // 将模拟推进一个时间步骤, 更新所有的圆圈位置和速度

    void render();                               // 渲染所有的加载的数据存储在 image 中

    void dumpParticles(const char* filename);    // 把当前的 position、velocity 和 radius dump 到文件

    void shadePixel(
        int circleIndex,
        float pixelCenterX, float pixelCenterY,
        float px, float py, float pz,
        float* pixelData);
};


#endif
