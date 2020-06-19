#include "Sphere.hpp"
#include "SampleHelperFunctions.hpp"

Intersection Sphere::GetIntersection(Ray ray, FaceCulling culling) {
	Intersection result;
	result.happened = false;
	Vector3f L = ray.origin - center;


	auto a = DotProduct(ray.direction, ray.direction);
	auto b = 2.0 * DotProduct(ray.direction, L);
	auto c = DotProduct(L, L) - radius2;
	float t0, t1;
	if (!SolveQuadratic(a, b, c, t0, t1)) return result;

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
		result.happened = true;
		result.coords = Vector3f(ray.origin + ray.direction * t_kept);
		result.normal = (result.coords - center).Normalized();
		result.m = this->m;
		result.obj = this;
		result.distance = t_kept;
	}
	return result;
}

Bounds3 Sphere::GetBounds() {
	return Bounds3(Vector3f(center.x - radius, center.y - radius, center.z - radius),
		Vector3f(center.x + radius, center.y + radius, center.z + radius));
}

void Sphere::Sample(Intersection& pos) {
	float theta = 2.0 * M_PI * GetRandomFloat(), phi = M_PI * GetRandomFloat();
	Vector3f dir(std::cos(phi), std::sin(phi) * std::cos(theta), std::sin(phi) * std::sin(theta));
	pos.coords = center + radius * dir;
	pos.normal = dir;
	pos.emit = m->GetEmission();
	pos.obj = this;
}
