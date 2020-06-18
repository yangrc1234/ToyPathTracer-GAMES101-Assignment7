//
// Created by LEI XU on 5/13/19.
//
#pragma once
#ifndef RAYTRACING_OBJECT_H
#define RAYTRACING_OBJECT_H

#include "Vector.hpp"
#include "global.hpp"
#include "Bounds3.hpp"
#include "Ray.hpp"
#include "Intersection.hpp"

enum FaceCulling {
    CullBack,
    CullFront,
    NoCull
};

class Object
{
public:
    Object(Material* m_) : m(m_) {}
    virtual ~Object() {}
    virtual Intersection getIntersection(Ray _ray, FaceCulling culling) = 0;
    virtual Bounds3 getBounds()=0;
    virtual float getArea()=0;
    virtual void Sample(Intersection &pos)=0;
    bool hasEmit() {
        return m->hasEmission();
    }
    //virtual Ray sampleLightStart(float* pdf);   //Shoot a ray from the object. used for bdpt.
    virtual float pdf() = 0;
    Material* m;
};



#endif //RAYTRACING_OBJECT_H
