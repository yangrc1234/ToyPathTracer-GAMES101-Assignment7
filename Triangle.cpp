#include "Triangle.hpp"
#include "OBJ_Loader.hpp"

bool rayTriangleIntersect(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector3f& orig, const Vector3f& dir, float& tnear, float& u, float& v)
{
	Vector3f edge1 = v1 - v0;
	Vector3f edge2 = v2 - v0;
	Vector3f pvec = CrossProduct(dir, edge2);
	float det = DotProduct(edge1, pvec);
	if (det == 0 || det < 0)
		return false;

	Vector3f tvec = orig - v0;
	u = DotProduct(tvec, pvec);
	if (u < 0 || u > det)
		return false;

	Vector3f qvec = CrossProduct(tvec, edge1);
	v = DotProduct(dir, qvec);
	if (v < 0 || u + v > det)
		return false;

	float invDet = 1 / det;

	tnear = DotProduct(edge2, qvec) * invDet;
	u *= invDet;
	v *= invDet;

	return true;
}

MeshTriangle::MeshTriangle(const std::string& filename, Material* m_) : Object(m_)
{
    objl::Loader loader;
    loader.LoadFile(filename);
    area = 0;
    assert(loader.LoadedMeshes.size() == 1);
    auto mesh = loader.LoadedMeshes[0];

    Vector3f min_vert = Vector3f{ std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max() };
    Vector3f max_vert = Vector3f{ -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max() };
    for (int i = 0; i < mesh.Vertices.size(); i += 3) {
        std::array<Vector3f, 3> face_vertices;

        for (int j = 0; j < 3; j++) {
            auto vert = Vector3f(mesh.Vertices[i + j].Position.X,
                mesh.Vertices[i + j].Position.Y,
                mesh.Vertices[i + j].Position.Z);
            face_vertices[j] = vert;

            min_vert = Vector3f(std::min(min_vert.x, vert.x),
                std::min(min_vert.y, vert.y),
                std::min(min_vert.z, vert.z));
            max_vert = Vector3f(std::max(max_vert.x, vert.x),
                std::max(max_vert.y, vert.y),
                std::max(max_vert.z, vert.z));
        }

        triangles.emplace_back(face_vertices[0], face_vertices[1],
            face_vertices[2], m);
    }

    bounding_box = Bounds3(min_vert, max_vert);

    std::vector<Object*> ptrs;
    for (auto& tri : triangles) {
        ptrs.push_back(&tri);
        area += tri.area;
    }
    bvh = new BVHAccel(ptrs);
}

Intersection Triangle::GetIntersection(Ray ray, FaceCulling culling)
{
    Intersection inter;

    if (culling == FaceCulling::CullBack) {
        if (DotProduct(ray.direction, normal) > 0)
            return inter;
    }
    else if (culling == FaceCulling::CullFront) {
        if (DotProduct(ray.direction, normal) < 0)
            return inter;
    }

    double u, v, t_tmp = 0;
    Vector3f pvec = CrossProduct(ray.direction, e2);
    double det = DotProduct(e1, pvec);
    if (fabs(det) < EPSILON)
        return inter;

    double det_inv = 1. / det;
    Vector3f tvec = ray.origin - v0;
    u = DotProduct(tvec, pvec) * det_inv;
    if (u < 0 || u > 1)
        return inter;
    Vector3f qvec = CrossProduct(tvec, e1);
    v = DotProduct(ray.direction, qvec) * det_inv;
    if (v < 0 || u + v > 1)
        return inter;
    t_tmp = DotProduct(e2, qvec) * det_inv;


    if (t_tmp < 0.0f)
        return inter;
    inter.distance = t_tmp;
    inter.coords = ray.origin + t_tmp * ray.direction;
    inter.obj = this;
    inter.normal = this->normal;
    inter.m = this->m;

    inter.happened = true;
    return inter;
}
