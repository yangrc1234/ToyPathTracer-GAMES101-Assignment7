#pragma once

#include "BVH.hpp"
#include "Intersection.hpp"
#include "Material.hpp"
#include "Object.hpp"
#include "Triangle.hpp"
#include <cassert>
#include <array>

bool rayTriangleIntersect(const Vector3f& v0, const Vector3f& v1,
    const Vector3f& v2, const Vector3f& orig,
    const Vector3f& dir, float& tnear, float& u, float& v);

class Triangle : public Object
{
public:
    inline Triangle(Vector3f _v0, Vector3f _v1, Vector3f _v2, Material* _m = nullptr)
        : Object(_m), v0(_v0), v1(_v1), v2(_v2)
    {
        e1 = v1 - v0;
        e2 = v2 - v0;
        normal = CrossProduct(e1, e2).Normalized();
        area = CrossProduct(e1, e2).Magnitude()*0.5f;
    }

    Intersection GetIntersection(Ray ray, FaceCulling culling) override;

    inline Bounds3 GetBounds() override { return Union(Bounds3(v0, v1), v2); }

    inline void Sample(Intersection &pos){
        float x = std::sqrt(GetRandomFloat()), y = GetRandomFloat();
        pos.coords = v0 * (1.0f - x) + v1 * (x * (1.0f - y)) + v2 * (x * y);
        pos.normal = this->normal;
        pos.obj = this;
    }

    inline float pdf() override {
        return 1.0f / area;
    }

    inline float getArea(){
        return area;
    }

    Vector3f v0, v1, v2; // vertices A, B ,C , counter-clockwise order
    Vector3f e1, e2;     // 2 edges v1-v0, v2-v0;
    Vector3f t0, t1, t2; // texture coords
    Vector3f normal;
    float area;
};

class MeshTriangle : public Object
{
public:
    MeshTriangle(const std::string& filename, Material* m_ = new Material());

    float pdf() override {
        return 1.0f / bvh->GN(bvh->Root()).area;
    }

    Bounds3 GetBounds() { return bounding_box; }

    inline Intersection GetIntersection(Ray ray, FaceCulling culling)
    {
        Intersection intersec;

        if (bvh) {
            intersec = bvh->Intersect(ray, culling);
        }

        return intersec;
    }
    
    inline void Sample(Intersection &pos){
        bvh->Sample(pos);
        pos.emit = m->GetEmission();
    }

    inline float getArea(){
        return area;
    }

    Bounds3 bounding_box;
    std::unique_ptr<Vector3f[]> vertices;
    uint32_t numTriangles;
    std::unique_ptr<uint32_t[]> vertexIndex;
    std::unique_ptr<Vector2f[]> stCoordinates;
    std::vector<Triangle> triangles;
    BVHAccel* bvh;
    float area;
};

