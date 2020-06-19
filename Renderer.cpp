//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <queue>
#include <future>
#include "Scene.hpp"
#include "Renderer.hpp"
#include "PathTracer.hpp"
#include "SceneRenderingHelper.hpp"
#include "BDPT.hpp"

using Buffer = std::vector<Vector3f>;
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

Buffer FillBufferThread(int threadCount, int threadOffset, int spp, Vector3f* buffer, bool bdpt) {
    float scale = CalculateScale(curScene->fov);
    float imageAspectRatio = curScene->width / (float)curScene->height;
    int pixelCount = curScene->width * curScene->height;
    int threadRayCounter = 0;
    Buffer emissionBuffer(curScene->width * curScene->height);
    for (size_t i = threadOffset; i < pixelCount; i+= threadCount)
    {
        int xPixel = i % curScene->width;
        int yPixel = i / curScene->width;
        ResetRandom((int)i + 1);
        for (int ispp = 0; ispp < spp; ispp++)
        {
            // generate primary ray direction
            Vector3f dir = PixelPosToRay(xPixel, yPixel, curScene->width, curScene->height, scale);
            int bounces;
            if (bdpt)
                *(buffer + i) += (1.0f / spp) * BDPT(curScene, Ray(curScene->eyePos, dir), bounces, &emissionBuffer[0]);
            else
                *(buffer + i) += (1.0f / spp) * PathTrace(curScene, Ray(curScene->eyePos, dir), bounces);
            threadRayCounter += bounces;
        }
        if (threadOffset == 0 && i % 10000 == 0){  //Logging IS performance issue, don't do too much.
            UpdateProgress((float)i / pixelCount);
        }
    }
    for (int i = 0; i < curScene->width * curScene->height; i++) {
        emissionBuffer[i] = emissionBuffer[i] * 1.0f / spp;
    }
    totalRays += threadRayCounter;
    return emissionBuffer;
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(std::string outputFileName, const Scene& scene, int spp, int thread_count, bool bdpt)
{
    if (bdpt) {
        std::cout << "Tracing mode: Bidirectional Ptah Tracing" << std::endl;
    }
    else {
        std::cout << "Tracing mode: Path tracing" << std::endl;
    }
    auto start = std::chrono::system_clock::now();
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    curScene = &scene;

    // change the spp value to change sample ammount
    std::cout << "SPP: " << spp << "\n";

    std::vector<std::future<Buffer>> threads;
    std::vector<Buffer> emissionBuffers;
    for (int iThread = 1; iThread < thread_count; iThread++)
    {
        auto future = std::async(std::launch::async, FillBufferThread, thread_count, iThread, spp, &framebuffer[0], bdpt);
        threads.push_back(std::move(future));
    }
    emissionBuffers.push_back(FillBufferThread(thread_count, 0, spp, &framebuffer[0], bdpt));

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].wait();
    }

    if (bdpt) {
        //Merge emission buffer.
        for (size_t i = 0; i < threads.size(); i++)
        {
            emissionBuffers.push_back(threads[i].get());
        }

        std::cout << "Tracing finished, merge emission buffer\n";
        for (size_t j = 0; j < scene.width * scene.height; j++)
        {
            //framebuffer[j] = 0.0f;
            for (size_t i = 0; i < emissionBuffers.size(); i++)
            {
                framebuffer[j] += emissionBuffers[i][j];
            }
        }
    }

    std::cout << std::endl;
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";
    std::cout << "Rays: " << totalRays << std::endl;
    std::cout << "Rays Per Second: " << (float)totalRays / 1e3f / std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "MRays" << std::endl;
    
    SaveFloatImageToJpg(framebuffer, scene.width, scene.height, outputFileName.c_str());
}
