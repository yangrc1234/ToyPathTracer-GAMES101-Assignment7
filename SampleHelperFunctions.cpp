#include "SampleHelperFunctions.hpp"
#include <cmath>

bool SolveQuadratic(const float& a, const float& b, const float& c, float& x0, float& x1)
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
Vector3f Reflect(Vector3f I, const Vector3f& N)
{
	I = -I;
	return I - 2 * DotProduct(I, N) * N;
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
Vector3f Refract(Vector3f I, const Vector3f& N, const float& ior)
{
	I = -I;
	float cosi = std::clamp(DotProduct(I, N), -1.0, 1.0);
	float etai = 1, etat = ior;
	Vector3f n = N;
	if (cosi < 0) { cosi = -cosi; }
	else { std::swap(etai, etat); n = -N; }
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? 0 : (eta * I + (eta * cosi - sqrtf(k)) * n).Normalized();
}

Vector3f AnyPerpendicular(Vector3f i) {
	//a*i.x + b * i.y + c * i.z == 0.0, solve any a,b,c.
	//Assume a = 0.0, b = 1.0, then c = -1.0 * i.y / i.z.
	//But if i.z == 0.0f, 
	if (i.z == 0.0f) {
		if (i.y == 0.0f) {
			return Vector3f(0.0f, 1.0f, 0.0f);
		}
		else {
			//Assume a = 1.0f, b = to_solve, c = 0.0f
			return Vector3f(1.0f, -i.x / i.y, 0.0f).Normalized();
		}
	}
	else {
		return Vector3f(0.0f, 1.0f, -1.0f * i.y / i.z).Normalized();
	}
}
