#pragma once
#include <iostream>
#include <cmath>
#include <random>
#include "Vector.hpp"

#undef M_PI
#define M_PI 3.141592653589793f
inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

extern const float  EPSILON;

const float kInfinity = std::numeric_limits<float>::max();

thread_local extern uint32_t s_RndState;

uint32_t XorShift32();

void ResetRandom(int seed);

float GetRandomFloat();

int GetRandom();

void UpdateProgress(float progress);