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

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior_d = 1.5f; //Only for dieletric.
    Vector3f ior_m = Vector3f(0.13100, 0.55758, 1.4561)  ,ior_m_k = Vector3f(4.0624, 2.2039, 1.9541); /*Only for metal. The default value is silver.*/;
    Vector3f Kd;
    float rough = 0.2f;

    void SetSmoothness(float smooth);
    Material(MaterialType t=Dieletric, Vector3f e=Vector3f(0,0,0));
    MaterialType getType();
    Vector3f getEmission();
    bool hasEmission();
    Vector3f fresnel(Vector3f I, const Vector3f& N) const;
    float pdf(Vector3f w_o, Vector3f n, Vector3f w_i);
    Vector3f sample(Vector3f w_o, Vector3f n, float* pdf);
    Vector3f evalGivenSample(const Vector3f& wo, const Vector3f& wi, const Vector3f& N, bool combineCosineTerm = true);
};


#endif //RAYTRACING_MATERIAL_H
