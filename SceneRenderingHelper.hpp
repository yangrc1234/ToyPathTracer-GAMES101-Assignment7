#pragma once

#include "global.hpp"
#include "Vector.hpp"
#include "Scene.hpp"
#include "SampleHelperFunctions.hpp"

enum BlendMode {
    Replace,
    Additive
};

float CalculateScale(float fov);

Vector3f PixelPosToRay(int xPixel, int yPixel, int width, int height, float scale);

Vector3f RayToUV(Ray ray, int width, int height, float scale);

void DrawToImage(Ray lightRay, Vector3f* buffer, Vector3f value, float fov, int width, int height, BlendMode mode = BlendMode::Additive);

void SaveFloatImageToJpg(std::vector<Vector3f> framebuffer, int width, int height, std::string path);