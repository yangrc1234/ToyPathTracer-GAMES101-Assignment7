//
// Created by LEI XU on 5/13/19.
//

#pragma once

#include "Object.hpp"
#include "Vector.hpp"
#include "Bounds3.hpp"
#include "Material.hpp"

class Sphere : public Object{
public:
    Vector3f center;
    float radius, radius2;
    float area;

    Sphere(const Vector3f &c, const float &r, Material* mt = new Material()) : center(c), radius(r), radius2(r * r), Object(mt), area(4 * M_PI *r *r) {}

    Intersection GetIntersection(Ray ray, FaceCulling culling);

    Bounds3 GetBounds();
    
    void Sample(Intersection& pos);

    inline float pdf() {
        return 1.0f / area;
    }

    inline float getArea(){
        return area;
    }
};