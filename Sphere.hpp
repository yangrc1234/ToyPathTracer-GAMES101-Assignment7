//
// Created by LEI XU on 5/13/19.
//

#ifndef RAYTRACING_SPHERE_H
#define RAYTRACING_SPHERE_H

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

    Intersection getIntersection(Ray ray, FaceCulling culling){
        Intersection result;
        result.happened = false;
        Vector3f L = ray.origin - center;


        auto a = dotProduct(ray.direction, ray.direction);
        auto b = 2.0 * dotProduct(ray.direction, L);
        auto c = dotProduct(L, L) - radius2;
        float t0, t1;
        if (!solveQuadratic(a, b, c, t0, t1)) return result;

        float t_kept;
        if (culling == FaceCulling::CullBack) {
            //Discard second solve
            t_kept = t0;
        }
        else if (culling == FaceCulling::CullFront) {
            //Discard first solve.
            t_kept = t1;
        }
        else {
            if (t0 <= 0)
                t_kept = t1;
            else
                t_kept = t0;
        }

        if (t_kept > 0.0f) {
            result.happened=true;
            result.coords = Vector3f(ray.origin + ray.direction * t_kept);
            result.normal = normalize(Vector3f(result.coords - center));
            result.m = this->m;
            result.obj = this;
            result.distance = t_kept;
        }
        return result;
    }

    Bounds3 getBounds(){
        return Bounds3(Vector3f(center.x-radius, center.y-radius, center.z-radius),
                       Vector3f(center.x+radius, center.y+radius, center.z+radius));
    }
    void Sample(Intersection &pos, float &pdf){
        float theta = 2.0 * M_PI * get_random_float(), phi = M_PI * get_random_float();
        Vector3f dir(std::cos(phi), std::sin(phi)*std::cos(theta), std::sin(phi)*std::sin(theta));
        pos.coords = center + radius * dir;
        pos.normal = dir;
        pos.emit = m->getEmission();
        pos.obj = this;
        pdf = 1.0f / area;
    }
    float pdf() {
        return 1.0f / area;
    }
    float getArea(){
        return area;
    }
};




#endif //RAYTRACING_SPHERE_H
