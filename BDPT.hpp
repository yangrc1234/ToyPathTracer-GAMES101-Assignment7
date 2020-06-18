#pragma once
#include "global.hpp"
#include "Scene.hpp"
#include <cassert>
#include "PTVertex.hpp"
#include "SampleHelperFunctions.hpp"

#define MAX_BDPT_PATH_LENGTH 16
#define RUSSIAN_ROULETTE 0.8f

struct BDPTPath;
struct PathVertex;
struct BDPTPathView;

struct BDPTPath {
    struct InternalPathVertex {
        PTVertex vertex;
        float pdf;
        Vector3f alpha;
        bool shadowed = false;
    };

    int count = 0;
    InternalPathVertex verts[MAX_BDPT_PATH_LENGTH * 2];
    const Scene* scene;

    operator BDPTPathView ();

    BDPTPath(const Scene* scene)
    {
        this->scene = scene;
    }

    PathVertex operator[](int i) const;

    PathVertex GetVertex(int i) const;

    void GenerateCameraPath(Ray cameraRay);

    void GenerateLightPath(Object* lightObj);

    void FillPathUsingRussianRoulette(int start);

    inline BDPTPath Copy(int count) const {
        assert(count <= this->count);
        BDPTPath t = *this;
        t.count = count;
        return t;
    }

    BDPTPathView Sub(int count);

    BDPTPath& Append(PTVertex vertex, bool dontcheckshadow);

    static Vector3f PathWeight(const BDPTPathView& lightPath, const BDPTPathView& camPath);

    inline InternalPathVertex SampleNextVertex(const Scene* scene, const InternalPathVertex& vertex, Vector3f w_o);
private:

};


struct PathVertex {
    int index;
    const BDPTPath* path;
    PathVertex(const BDPTPath* path, int index)
    {
        this->path = path;
        this->index = index;
    }
    PathVertex() :path(nullptr)
    {
    }

    inline PathVertex Pre() const {
        assert(path);
        assert(index > 0);
        return path->GetVertex(index - 1);
    }

    inline PathVertex Next() const {
        assert(path);
        assert(path->count > index + 1);
        return path->GetVertex(index + 1);
    }

    inline Vector3f Position() const {
        return path->verts[index].vertex.x;
    }

    inline Vector3f Normal() const {
        return path->verts[index].vertex.N;
    }

    inline float Pdf() const {
        return path->verts[index].pdf;
    }

    inline Vector3f Throughput()const {
        return path->verts[index].alpha;
    }

    inline PTVertex Vertex() const {
        return path->verts[index].vertex;
    }

    inline PTVertex::Type Type() const {
        return path->verts[index].vertex.type;
    }

    inline Material* Material() const {
        if (path->verts[index].vertex.obj != nullptr)
            return path->verts[index].vertex.obj->m;
        return nullptr;
    }

    inline Vector3f Emission() const {
        if (Material() == nullptr)
            return 0.0f;
        else {
            if (this->Type() == PTVertex::Type::Background)
                return path->scene->backgroundColor;
            else
                return Material()->m_emission;
        }
    }

    inline bool IsLightPath() const {
        if (path->count == 0)
            return false;
        return path->GetVertex(0).Type() == PTVertex::Type::Light;
    }

    Vector3f EvalBsdfOnSolidAngle(Vector3f dir) {
        assert(Type() != PTVertex::Type::Background);

        if (Type() == PTVertex::Type::Light) {
            return 1.0f;
        }
        else if (Type() == PTVertex::Type::Camera) {
            return 1.0f;
        } else {
            assert(index >= 1);
            return Material()->evalGivenSample((Pre().Position() - Position()).normalized(), dir, Normal(), false);
        }
    }

    float EvalPdfOnSolidAngle(Vector3f dir) {
        assert(Type() != PTVertex::Type::Background);

        if (Type() == PTVertex::Type::Light) {
            return getCosineWeightedPdf(Normal(), dir);
        }
        else if (Type() == PTVertex::Type::Camera) {
            return 1.0f;
        }
        else {
            float cosine = abs(dotProduct(dir, Normal()));
            if (cosine == 0.0f)
                return 0.0f;
            Vector3f wo = (Pre().Position() - Position()).normalized();
            return Material()->pdf(
                wo,
                Normal(),
                dir) / cosine;
        }
    }

    float EvalPdfOnAreaSurface(const Scene* scene, PTVertex v) {
        float distSqr;
        Vector3f w_i = (v.x - Position()).NormlizeAndGetLengthSqr(&distSqr);

        assert(Type() != PTVertex::Type::Background);
        float srpdf = EvalPdfOnSolidAngle(w_i);
        if (scene->ShadowCheck(Vertex(), v)) {
            return 0.0f;
        }
        else {
            return SrpdfToAreaPdf(srpdf, Vertex(), v);
        }
    }

    Vector3f Sample(float* pdf) {
        assert(Type() != PTVertex::Type::Camera);
        assert(Type() != PTVertex::Type::Background);


        if (Type() == PTVertex::Type::Light) {
            return getCosineWeightedSample(Normal(), *pdf);
        }
        else {
            Vector3f w_o = (Pre().Position() - Position()).normalized();
            return Material()->sample(w_o, Normal(), pdf);
        }
    }
};


struct BDPTPathView {
    const BDPTPath* subpath;
    int count;
    BDPTPathView(const BDPTPath* subpath, int count)
    {
        this->subpath = subpath;
        assert(count <= subpath->count);
        this->count = count;
    }

    const PathVertex operator[](int i) const {
        return V(i);
    }

    PathVertex V(int index) const {
        assert(index < count);
        return subpath->GetVertex(index);
    }

    PathVertex Last() const {
        assert(count > 0);
        return subpath->GetVertex(count - 1);
    }
};


Vector3f BDPT(const Scene* scene, const Ray& ray, int& outBounces, Vector3f* emissionBuffer = nullptr);