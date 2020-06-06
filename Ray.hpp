//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_RAY_H
#define RAYTRACING_RAY_H
#include "Vector.hpp"
struct Ray{
    //Destination = origin + t*direction
    Vector3f origin;
    Vector3f direction, direction_inv;
    
    Ray(const Vector3f& ori, const Vector3f& dir, const double _t = 0.0): origin(ori), direction(dir) {
        direction_inv = Vector3f(1./direction.x, 1./direction.y, 1./direction.z);

    }

    Vector3f operator()(float t) const{return origin+direction*t;}

    friend std::ostream &operator<<(std::ostream& os, const Ray& r){
        os<<"[origin:="<<r.origin<<", direction="<<r.direction<<"]\n";
        return os;
    }
};
#endif //RAYTRACING_RAY_H
