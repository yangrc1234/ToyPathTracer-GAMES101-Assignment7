#include "global.hpp"



uint32_t XorShift32()
{
	uint32_t x = s_RndState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 15;
	s_RndState = x;
	return x;
}

void ResetRandom(int seed = 1) {
	s_RndState = seed;
}

float GetRandomFloat()
{
	return (double)(XorShift32()) / 0xffffffff;
}

int GetRandom()
{
	return XorShift32();
}

void UpdateProgress(float progress)
{
	printf("Generating: %.1f%%\r", 100.0f * progress);
}
