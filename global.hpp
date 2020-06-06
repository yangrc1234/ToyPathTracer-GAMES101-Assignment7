#pragma once
#include <iostream>
#include <cmath>
#include <random>
#include "Vector.hpp"

#undef M_PI
#define M_PI 3.141592653589793f

extern const float  EPSILON;
const float kInfinity = std::numeric_limits<float>::max();

float clamp(const float& lo, const float& hi, const float& v);

bool solveQuadratic(const float& a, const float& b, const float& c, float& x0, float& x1);


// Compute reflection direction
Vector3f reflect(Vector3f I, const Vector3f& N);

// Compute refraction direction using Snell's law
//
// We need to handle with care the two possible situations:
//
//    - When the ray is inside the object
//
//    - When the ray is outside.
//
// If the ray is outside, you need to make cosi positive cosi = -N.I
//
// If the ray is inside, you need to invert the refractive indices and negate the normal N
Vector3f refract(Vector3f I, const Vector3f& N, const float& ior);

Vector3f anyPerpendicular(Vector3f i);

thread_local extern uint32_t s_RndState;
uint32_t XorShift32();

void reset_random(int seed);

float get_random_float();

int get_random();

void UpdateProgress(float progress);