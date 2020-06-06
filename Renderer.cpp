//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <queue>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <future>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 1e-4;
struct ThreadTask {
    ThreadTask(Vector3f* target, int x, int y) :
        target(target),
        x(x),y(y)
     {}
    int x,y;
    Vector3f* target;
};

const Scene* curScene;
std::atomic<int> totalRays;

float CalculateScale(float fov) {
    return tan(deg2rad(fov * 0.5));
}

Vector3f PixelPosToRay(int xPixel, int yPixel, int width, int height, float scale) {
    float imageAspectRatio = width / height;
    float x = (2 * (xPixel + 0.5) / (float)width - 1) *
        imageAspectRatio * scale;
    float y = (1 - 2 * (yPixel + 0.5) / (float)height) * scale;
    return normalize(Vector3f(-x, y, 1));
}

void FillBufferThread(int threadCount, int threadOffset, int spp, Vector3f* buffer) {
    float scale = CalculateScale(curScene->fov);
    float imageAspectRatio = curScene->width / (float)curScene->height;
    int pixelCount = curScene->width * curScene->height;
    int threadRayCounter = 0;
    for (size_t i = threadOffset; i < pixelCount; i+= threadCount)
    {
        int xPixel = i % curScene->width;
        int yPixel = i / curScene->width;
        reset_random((int)i);
        for (int ispp = 0; ispp < spp; ispp++)
        {
            // generate primary ray direction
            Vector3f dir = PixelPosToRay(xPixel, yPixel, curScene->width, curScene->height, scale);
            int bounces;
            *(buffer + i) += (1.0f / spp) * curScene->castRay(Ray(curScene->eyePos, dir), bounces);
            threadRayCounter += bounces;
        }
        if (threadOffset == 0 && i % 10000 == 0){  //Logging IS performance issue, don't do too much.
            UpdateProgress((float)i / pixelCount);
        }
    }
    totalRays += threadRayCounter;
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene, int spp, int thread_count)
{
    auto start = std::chrono::system_clock::now();
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    curScene = &scene;

    // change the spp value to change sample ammount
    std::cout << "SPP: " << spp << "\n";

    std::vector<std::future<void>> threads;
    for (int iThread = 1; iThread < thread_count; iThread++)
    {
        auto future = std::async(std::launch::async, FillBufferThread, thread_count, iThread, spp, &framebuffer[0]);
        threads.push_back(std::move(future));
    }
    FillBufferThread(thread_count, 0, spp, &framebuffer[0]);

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].wait();
    }
    std::cout << std::endl;
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";
    std::cout << "Rays: " << totalRays << std::endl;
    std::cout << "Rays Per Second: " << (float)totalRays / 1e3f / std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "MRays" << std::endl;
    
    std::vector<unsigned char> normalizedBuffer;
    normalizedBuffer.reserve(framebuffer.size() * 3);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        normalizedBuffer.insert(normalizedBuffer.end(), color, color + 3);
    }

    stbi_write_jpg("output.jpg", scene.width, scene.height, 3, &normalizedBuffer[0], 100);
    //stbi_write_png("output.png", scene.width, scene.height, 3, &normalizedBuffer[0], 0);
    /*
    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    */
}
