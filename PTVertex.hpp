#pragma once
#include "Vector.hpp"
#include "Object.hpp"
#include <cassert>

struct PTVertex {
    enum class Type {
        Background,
        Intermediate,
        Light,
        Camera
    };
    Type type;
    Vector3f x;
    Vector3f N;
    Object* obj = nullptr;

    static PTVertex Camera(Vector3f cameraPos);

    static PTVertex Background();
};
