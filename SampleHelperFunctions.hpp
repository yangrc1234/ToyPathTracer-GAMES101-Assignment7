#pragma once
#include "Vector.hpp"
#include "PTVertex.hpp"

/*Clamp a value to range [0,1]*/
inline float saturate(float t) {
	return std::clamp(t, 0.0f, 1.0f);
}

/*Power5 of a value*/
inline float pow5(float v) {
	return v * v * v * v * v;
}

inline float lerpf(float a, float b, float t) {
	return (1.0f - t) * a + t * b;
}

template <typename T, typename TPdf>
T SafeDivide(T v, TPdf pdf) {
	if (pdf == 0.0f) {
		return 0.0f;
	}
	else {
		return v / pdf;
	}
}

/*
Given a vector a that is located in local system, transform the local system to world system such that z-axis in local system aligns with world system's one.
*/
inline Vector3f toWorld(const Vector3f& a, const Vector3f& N) {
	Vector3f tangent = anyPerpendicular(N), bitangent = crossProduct(N, tangent);

	return Vector3f(
		a.x * tangent.x + a.y * bitangent.x + a.z * N.x,
		a.x * tangent.y + a.y * bitangent.y + a.z * N.y,
		a.x * tangent.z + a.y * bitangent.z + a.z * N.z
		);
}

/*Given N, w_i, w_o and material ior, get ior of the media w_i in, and ior of the media w_o in*/
inline void GetInsideOutsideIOR(Vector3f N, Vector3f wi, Vector3f wo, float matIor, float& ior_i, float& ior_o) {
	float nl = dotProduct(N, wi);
	float nv = dotProduct(N, wo);
	//Calculate half dir.
	if (nl < 0.0f) {
		ior_i = matIor;
	}
	else {
		ior_i = 1.0f;
	}
	if (nv < 0.0f) {
		ior_o = matIor;
	}
	else {
		ior_o = 1.0f;
	}
}

/*
Given normal, w_i, w_o and material ior, calculate half vector.
If w_i, w_o is on the same side, (w_i + w_o).normalized().
Otherwise refraction is considered, and the half direction is calculated according t othe GGX paper.(Which is exactly normal of the plane that refracts the w_i or w_o into one another)*/
inline Vector3f GetHalfDir(Vector3f N, Vector3f wi, Vector3f wo, float matIor) {

	float nl = dotProduct(N, wi);
	float nv = dotProduct(N, wo);
	if (nl == 0.0f || nv == 0.0f)
		return 0.0f;
	Vector3f h;
	if (nl * nv > 0.0f)      //Reflection.
	{
		h = (wi + wo).normalized();
		if (nv < 0.0f)
			h = -h;
	}
	else {
		//Calculate half dir.
		if (nv < 0.0f) {
			h = -(matIor * wo + wi).normalized();
		}
		else {
			h = -(wo + wi * matIor).normalized();
		}
	}
	return h;
}

/*Draw a sample from diffuse distribution, which is cosine-weighted hemisphere. */
inline Vector3f getCosineWeightedSample(const Vector3f& N, float& pdf) {
	//cosine-weighted sampling.
	float u1 = get_random_float();
	float r = std::sqrt(u1);
	float theta = 2 * M_PI * get_random_float();
	float x = r * cos(theta), y = r * sin(theta);

	Vector3f wi = toWorld(Vector3f(x, y, std::sqrt(1.0f - u1)), N).normalized();
	pdf = dotProduct(wi, N) / (M_PI);
	return wi;
}

/*Given w_o, N, w_i, calculate its probablity under Lambert lighting model.*/
inline float getCosineWeightedPdf(const Vector3f& N, const Vector3f& wi) {
	return saturate(dotProduct(wi, N)) / (M_PI); //cosine-weighted.
}

inline float SrpdfToAreaPdf(float srpdf, const PTVertex& v1, const PTVertex& v2) {
	assert(v1.type != PTVertex::Type::Background);
	float distSqr;
	Vector3f w = (v2.x - v1.x).NormlizeAndGetLengthSqr(&distSqr);
	if (v1.type == PTVertex::Type::Camera) {
		return srpdf * abs(dotProduct(-w, v2.N) / distSqr);
	}
	else {
		return srpdf * abs(dotProduct(w, v1.N) * dotProduct(-w, v2.N) / distSqr);
	}
}
