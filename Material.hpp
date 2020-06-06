//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"
#include "global.hpp"


enum MaterialType {
    Dieletric, Metal, TransmittanceDieletric
};

class Material{
public:
    class BSDFSampler {
    public:
        BSDFSampler(Material* mat)
        {
            this->mat = mat;
        }
        Material* mat;
        float pdf(Vector3f x, Vector3f w_o, Vector3f n, Vector3f w_i);
        Vector3f sample(Vector3f x, Vector3f w_o, Vector3f n, float* pdf);
    };

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior_d = 1.5f; //Only for dieletric.
    Vector3f ior_m = Vector3f(0.13100, 0.55758, 1.4561)  ,ior_m_k = Vector3f(4.0624, 2.2039, 1.9541); /*Only for metal. The default value is silver.*/;
    Vector3f Kd;

    BSDFSampler GetSampler() {
        return BSDFSampler(this);
    }

    float rough = 0.2f;

    void SetSmoothness(float smooth);
    
    //Calculate Fresnel term of the material, given incoming direction and normal.
    Vector3f fresnel(Vector3f I, const Vector3f& N) const;
    
    Material(MaterialType t=Dieletric, Vector3f e=Vector3f(0,0,0));
    MaterialType getType();

    Vector3f getEmission();
    bool hasEmission();

    //Given a explicit light ray, calculate the explicit contribution.
    Vector3f evalGivenSample(const Vector3f& wo, const Vector3f& wi, const Vector3f& N);
};


#endif //RAYTRACING_MATERIAL_H
