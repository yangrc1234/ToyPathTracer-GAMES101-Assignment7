//
// Created by Göksu Güvendiren on 2019-05-14.
//

#pragma once

#include <vector>

#include "Vector.hpp"
#include "Object.hpp"
#include "BVH.hpp"
#include "Ray.hpp"
#include "PTVertex.hpp"

class Scene
{
public:
    // setting up options
    int width = 1280;
    int height = 960;
    double fov = 40;
    Vector3f eyePos;
    Vector3f backgroundColor = Vector3f(0.235294f, 0.67451f, 0.843137f);
    int maxDepth = 1;
    float RussianRoulette = 0.8;
    BVHAccel* bvh;
    std::vector<Object*> objects;
    std::vector<Object*> m_emissionObjects;

    Scene(int w, int h) : width(w), height(h)
    {}

    Scene& Add(Object* object) { objects.push_back(object);  return *this; }
    const std::vector<Object*>& GetObjects() const { return objects; }
    PTVertex Intersect(const Ray& ray, FaceCulling culling = FaceCulling::CullBack) const;
    void BuildBVH();
    bool ShadowCheck(Vector3f lightCoords, Vector3f x, FaceCulling culling = CullBack) const;
    bool ShadowCheck(const PTVertex& v1, const PTVertex& v2) const;

};