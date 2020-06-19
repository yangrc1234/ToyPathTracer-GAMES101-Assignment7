#pragma once

#include "SceneRenderingHelper.hpp"

#include "global.hpp"
#include "Vector.hpp"
#include "Scene.hpp"
#include "SampleHelperFunctions.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

float CalculateScale(float fov) {
    return tan(deg2rad(fov * 0.5));
}

Vector3f PixelPosToRay(int xPixel, int yPixel, int width, int height, float scale) {
    float imageAspectRatio = width / height;
    float x = (2 * (xPixel + 0.5) / (float)width - 1) *
        imageAspectRatio * scale;
    float y = (1 - 2 * (yPixel + 0.5) / (float)height) * scale;
    return Vector3f(-x, y, 1).Normalized();
}

Vector3f RayToUV(Ray ray, int width, int height, float scale) {
    float imageAspectRatio = width / height;
    auto t = Vector3f(-ray.direction.x / scale / imageAspectRatio, -ray.direction.y / scale, 0.0f);
    return (t + 1.0f) * 0.5f;
}

void DrawToImage(Ray lightRay, Vector3f* buffer, Vector3f value, float fov, int width, int height, BlendMode mode) {
    lightRay.direction = lightRay.direction / lightRay.direction.z;
    Vector3f uv = RayToUV(lightRay, width, height, CalculateScale(fov));
    Vector3f coordScreenSpace = Vector3f(uv.x * width, uv.y * height, 0.0f);

    int centerPixelX = coordScreenSpace.x;
    int centerPixelY = coordScreenSpace.y;

    for (int ix = centerPixelX - 1; ix <= centerPixelX + 1; ix++) {
        for (int iy = centerPixelY - 1; iy <= centerPixelY + 1; iy++) {
            if (ix < 0 || iy < 0 || ix >= width || iy >= height)
                continue;
            Vector3f curPixelUV = Vector3f(ix + 0.5f, iy + 0.5f, 0.0f);
            float distanceX = std::abs(coordScreenSpace.x - curPixelUV.x);
            float distanceY = std::abs(coordScreenSpace.y - curPixelUV.y);

            float weight =
                std::max(0.0f, 1.0f - distanceX)
                * std::max(0.0f, 1.0f - distanceY);
            if (mode == BlendMode::Additive)
                buffer[ix + height * iy] += weight * value;
            else
                buffer[ix + height * iy] = weight * value;
        }
    }
}

void SaveFloatImageToJpg(std::vector<Vector3f> framebuffer, int width, int height, std::string path) {
    std::vector<unsigned char> normalizedBuffer;
    normalizedBuffer.reserve(framebuffer.size() * 3);
    for (auto i = 0; i < height * width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(std::clamp(framebuffer[i].x, 0.f, 1.f), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(std::clamp(framebuffer[i].y, 0.f, 1.f), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(std::clamp(framebuffer[i].z, 0.f, 1.f), 0.6f));
        normalizedBuffer.insert(normalizedBuffer.end(), color, color + 3);
    }

    stbi_write_jpg(path.c_str(), width, height, 3, &normalizedBuffer[0], 100);

}
