#pragma once
#include <cmath>
#include "Vector.hpp"
#include "SampleHelperFunctions.hpp"
#include "global.hpp"

/*GGX Geometry shadow term. */
inline float Visibility(float vn, float vh, float roughness) {
	if (vh * vn <= 0.0f)
		return 0.0f;
	float vh2 = vh * vh;
	float tan2 = (1.0f - vh2) / vh2;
	return 2.0f / (1 + std::sqrt(1.0f + roughness * roughness * tan2));
}

/*GGX D term. Indicate given N, H, roughness, how much */
inline float GGXTerm(float ndoth, float roughness) {
	float a2 = roughness * roughness;
	float costheta = ndoth;
	float costheta2 = costheta * costheta;
	float cosehta4 = costheta2 * costheta2;

	float tangenttheta2 = (1.0f - costheta2) / costheta2;
	float denominator_partb = a2 + tangenttheta2;
	denominator_partb = denominator_partb * denominator_partb;

	return a2 / (
		M_PI * cosehta4 * denominator_partb
		);
	/*
	float d = (ndoth * a2 - ndoth) * ndoth + 1.0f;
	return a2 / (M_PI * d * d + EPSILON);
	*/


}

/*Given normal and microsurface normal(half direction), calculate probablity of the microsurface.*/
inline float GGXHalfPDF(Vector3f n, Vector3f h, float roughness) {
	return GGXTerm(abs(dotProduct(n, h)), roughness) * abs(dotProduct(n, h));
}

/*Convert smoothness to roughness*/
inline float SmoothnessToRoughenss(float smoothness) {
	return std::max(0.002f, (1.0f - smoothness) * (1.0f - smoothness));
}

/*
Given normal and roughness, generate a half dir sample.
GGX paper(35)(36)
*/
inline Vector3f SampleGGXSpecularH(Vector3f N, float roughness) {
	float d1 = get_random_float(), d2 = get_random_float();
	float theta = std::atan2(roughness * std::sqrt(d1), std::sqrt(1.0f - d1));
	float phi = 2.0f * M_PI * d2;
	Vector3f microNLocal(
		std::sin(theta) * std::cos(phi),
		std::sin(theta) * std::sin(phi),
		std::cos(theta)
		);

	auto microNWorld = toWorld(microNLocal, N).normalized();

	return microNWorld;
}
