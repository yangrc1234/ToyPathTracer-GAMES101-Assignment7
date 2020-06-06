#include "global.hpp"



float clamp(const float& lo, const float& hi, const float& v)
{
	return std::max(lo, std::min(hi, v));
}

bool solveQuadratic(const float& a, const float& b, const float& c, float& x0, float& x1)
{
	double discr = (double)b * b - 4.0 * a * c;
	if (discr < 0) return false;
	else if (discr == 0) x0 = x1 = -0.5 * b / a;
	else {
		float q = (b > 0) ?
			-0.5 * (b + sqrt(discr)) :
			-0.5 * (b - sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}
	if (x0 > x1) std::swap(x0, x1);
	return true;
}

// Compute reflection direction
Vector3f reflect(Vector3f I, const Vector3f& N)
{
	I = -I;
	return I - 2 * dotProduct(I, N) * N;
}

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
Vector3f refract(Vector3f I, const Vector3f& N, const float& ior)
{
	I = -I;
	float cosi = clamp(-1, 1, dotProduct(I, N));
	float etai = 1, etat = ior;
	Vector3f n = N;
	if (cosi < 0) { cosi = -cosi; }
	else { std::swap(etai, etat); n = -N; }
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? 0 : (eta * I + (eta * cosi - sqrtf(k)) * n).normalized();
}

Vector3f anyPerpendicular(Vector3f i) {
	//a*i.x + b * i.y + c * i.z == 0.0, solve any a,b,c.
	//Assume a = 0.0, b = 1.0, then c = -1.0 * i.y / i.z.
	//But if i.z == 0.0f, 
	if (i.z == 0.0f) {
		if (i.y == 0.0f) {
			return Vector3f(0.0f, 1.0f, 0.0f);
		}
		else {
			//Assume a = 1.0f, b = to_solve, c = 0.0f
			return Vector3f(1.0f, -i.x / i.y, 0.0f).normalized();
		}
	}
	else {
		return Vector3f(0.0f, 1.0f, -1.0f * i.y / i.z).normalized();
	}
}

uint32_t XorShift32()
{
	uint32_t x = s_RndState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 15;
	s_RndState = x;
	return x;
}

void reset_random(int seed = 1) {
	s_RndState = seed;
}

float get_random_float()
{
	return (double)(XorShift32()) / 0xffffffff;
}

int get_random()
{
	return XorShift32();
}

void UpdateProgress(float progress)
{
	printf("Generating: %.1f%%\r", 100.0f * progress);
}
