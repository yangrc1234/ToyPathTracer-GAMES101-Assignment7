#include "global.hpp"
#include "Vector.hpp"
#include "BDPT.hpp"
#include "SampleHelperFunctions.hpp"
#include "SceneRenderingHelper.hpp"

PTVertex PTVertex::Camera(Vector3f cameraPos) {
    PTVertex v;
	v.type = PTVertex::Type::Camera;
	v.x = cameraPos;
	return v;
}

PTVertex PTVertex::Background() {
    PTVertex v;
	v.type = PTVertex::Type::Background;
	return v;
}

BDPTPath::operator BDPTPathView() {
    return BDPTPathView(this, count);
}

PathVertex BDPTPath::operator[](int i) const {
    return PathVertex(this, i);
}
PathVertex BDPTPath::GetVertex(int i) const {
    return PathVertex(this, i);
}

inline void BDPTPath::GenerateCameraPath(Ray cameraRay) {
    verts[0].vertex = PTVertex::Camera(cameraRay.origin);
    verts[0].pdf = 1.0f;
    verts[0].alpha = Vector3f::One();
    verts[0].shadowed = false;

    verts[1].vertex = scene->Intersect(cameraRay);
    float distSqr;
    Vector3f atob = (verts[0].vertex.x - verts[1].vertex.x).NormlizeAndGetLengthSqr(&distSqr);
    verts[1].pdf = abs(DotProduct(-atob, verts[1].vertex.N)) / (EPSILON + distSqr);
    verts[1].alpha = Vector3f::One();
    verts[1].shadowed = false;

    if (verts[1].vertex.type == PTVertex::Type::Background) {
        count = 2;
        return;
    }
    FillPathUsingRussianRoulette(1);
}

void BDPTPath::GenerateLightPath(Object* lightObj) {
    Intersection t;
    lightObj->Sample(t);
    verts[0].vertex.x = t.coords;
    verts[0].vertex.type = PTVertex::Type::Light;
    verts[0].vertex.obj = t.obj;
    verts[0].vertex.N = t.normal;
    verts[0].shadowed = false;
    verts[0].pdf = lightObj->pdf();

    verts[0].alpha = lightObj->m->m_emission / verts[0].pdf;
    
    float pdf1;
    Vector3f w_i = GetCosineWeightedSample(t.normal, pdf1);

    float costheta = DotProduct(verts[0].vertex.N, w_i);
    pdf1 = SafeDivide(pdf1, costheta);

    verts[1].vertex = scene->Intersect(Ray(verts[0].vertex.x, w_i));
    verts[1].shadowed = false;
    verts[1].pdf = SrpdfToAreaPdf(pdf1, verts[0].vertex, verts[1].vertex);

    if (pdf1 != 0.0f)
        verts[1].alpha = SafeDivide(verts[0].alpha, pdf1);
    else if (verts[1].vertex.type == PTVertex::Type::Background) {
        count = 2;
        return;
    }
    FillPathUsingRussianRoulette(1);
}

void BDPTPath::FillPathUsingRussianRoulette(int start) {
    count = start + 1;

    for (int i = start; i < MAX_BDPT_PATH_LENGTH - 1; i++) {
        if (verts[i].vertex.type == PTVertex::Type::Background)
            break;
        auto w_o = verts[i - 1].vertex.x - verts[i].vertex.x;
        w_o = w_o.Normalized();
        auto mat = verts[i].vertex.obj->m;
        
        Vector3f bsdf;
        verts[i + 1] = SampleNextVertex(scene, verts[i], w_o);
        
        float rrProb = i > 4 ? RUSSIAN_ROULETTE : std::max(verts[i + 1].alpha.x, std::max(verts[i + 1].alpha.y, verts[i + 1].alpha.z));
        rrProb = std::min(rrProb, 1.0f);
        if (GetRandomFloat() > rrProb) {
            break;
        }
        
        if (verts[i + 1].pdf == 0.0f)
            break;

        verts[i + 1].pdf *= rrProb;
        verts[i + 1].alpha = verts[i].alpha * verts[i + 1].alpha / rrProb;

        count++;
    }
}


BDPTPathView BDPTPath::Sub(int count) {
    return BDPTPathView(this, count);
}

BDPTPath& BDPTPath::Append(PTVertex vertex, bool dontcheckshadow) {

    if (count == 0)
    {
        assert(vertex.type == PTVertex::Type::Camera || vertex.type == PTVertex::Type::Light);
        verts[0].vertex = vertex;
        verts[0].shadowed = false;
        verts[0].pdf = vertex.obj->pdf();
        if (vertex.type == PTVertex::Type::Light)
            verts[0].alpha = vertex.obj->m->m_emission/ verts[0].pdf;
        else
            verts[0].alpha = 1.0f;
        count++;
        return *this;
    }

    auto& lastVertex = this->operator[](count - 1);
    assert(lastVertex.Type() != PTVertex::Type::Background);

    auto& editVertex = verts[count];
    editVertex.vertex = vertex;

    editVertex.shadowed = dontcheckshadow ? false : scene->ShadowCheck(editVertex.vertex, lastVertex.Vertex());
    if (editVertex.shadowed)
    {
        editVertex.pdf = 0.0f;
        editVertex.alpha = 0.0f;
    }
    else {
        float distSqr;
        auto w_i = (vertex.x - lastVertex.Position()).NormlizeAndGetLengthSqr(&distSqr);
        
        float srpdf = lastVertex.EvalPdfOnSolidAngle(w_i);
        editVertex.pdf = SrpdfToAreaPdf(srpdf, lastVertex.Vertex(), editVertex.vertex);
        Vector3f alpha = SafeDivide(lastVertex.EvalBsdfOnSolidAngle(w_i), srpdf);
        editVertex.alpha = lastVertex.Throughput() * alpha;

        float rrProb = count > 4 ? RUSSIAN_ROULETTE : std::max(alpha.x, std::max(alpha.y, alpha.z));
        rrProb = std::min(rrProb, 1.0f);
        editVertex.pdf *= rrProb;
        editVertex.alpha = SafeDivide(editVertex.alpha, rrProb);
    }

    count++;

    return *this;
}

Vector3f BDPTPath::PathWeight(const BDPTPathView& lightPath, const BDPTPathView& camPath) {
    auto scene = lightPath.path->scene;
    assert(lightPath.count == 0 || lightPath.path->verts[0].vertex.type == PTVertex::Type::Light);
    assert(camPath.count >= 1 && camPath.path->verts[0].vertex.type == PTVertex::Type::Camera);
    assert(lightPath.count + camPath.count >= 2);

    auto z1 = camPath.Last();
    if (camPath.Last().Type() == PTVertex::Type::Background) {
        if (lightPath.count == 0)
            return camPath.Last().Throughput() * scene->backgroundColor;
        else
            return 0.0f;
    }
    if (lightPath.count != 0 && lightPath.Last().Type() == PTVertex::Type::Background) {
        return 0.0f;
    }

    Vector3f c_st;
    if (lightPath.count == 0) {
        auto z2 = camPath.Last().Pre();
        Vector3f w_i = (z2.Position() - z1.Position()).Normalized();
        c_st = z1.Emission() * DotProduct(z1.Normal(), w_i) ;
        if (DotProduct(z1.Emission(), z1.Emission()) == 0.0f)
            return 0.0f;
    }
    else if (camPath.count == 0) {
        return 0.0f;
    }
    else {
        float distSqr;
        Vector3f dir_ltoc = (camPath.Last().Position() - lightPath.Last().Position()).NormlizeAndGetLengthSqr(&distSqr);
        bool noshadowed = !scene->ShadowCheck(camPath.Last().Vertex(), lightPath.Last().Vertex());
        if (!noshadowed) {
            return 0.0f;
        }
        auto lightLast = lightPath.Last(); auto camLast = camPath.Last();
        float cosThetaCamera = camLast.index == 0 ? 1.0f : DotProduct(camLast.Normal(), -dir_ltoc);
        c_st =
            lightLast.EvalBsdfOnSolidAngle(dir_ltoc)
            * camLast.EvalBsdfOnSolidAngle(-dir_ltoc)
            * abs(
                DotProduct(lightLast.Normal(), dir_ltoc)
               * cosThetaCamera
                / distSqr
                );
    }

    //Calculate mis weight.
    float weightdenominator = 1.0f;
    
    //Copy a camera path to append those light verticies.
    BDPTPath temp = camPath.path->Copy(camPath.count);
    float cur_pdf = 1.0f;

    for (int i = lightPath.count - 1; i >= 0; i--) {
        temp.Append(lightPath[i].Vertex(), true);
        float pdf = temp[temp.count - 1].Pdf();
        cur_pdf *= SafeDivide(pdf, lightPath[i].Pdf());
        weightdenominator += cur_pdf * cur_pdf;
        if (cur_pdf == 0.0f)
            break;
    }

    temp = lightPath.path->Copy(lightPath.count);
    cur_pdf = 1.0f;
    for (int i = camPath.count - 1; i >= 0; i--) {
        if (temp.count == 0) {
            assert(DotProduct(camPath[i].Emission(), camPath[i].Emission()) != 0.0f);
            auto toAppend = camPath[i].Vertex();
            toAppend.type = PTVertex::Type::Light;
            temp.Append(toAppend, true);
        }
        else {
            temp.Append(camPath[i].Vertex(), true);
        }

        float pdf = temp[temp.count - 1].Pdf();
        cur_pdf *= SafeDivide(pdf, camPath[i].Pdf());
        weightdenominator += cur_pdf * cur_pdf;
        if (cur_pdf == 0.0f)
            break;
    }

    Vector3f lightThroughput = lightPath.count == 0 ? 1.0f : lightPath.Last().Throughput();

    Vector3f unweightedC = lightThroughput * camPath.Last().Throughput() * c_st;
    return unweightedC / weightdenominator;
}

BDPTPath::InternalPathVertex  BDPTPath::SampleNextVertex(const Scene* scene, const InternalPathVertex& vertex, Vector3f w_o) {
    assert(vertex.vertex.type != PTVertex::Type::Camera);
    assert(vertex.vertex.type != PTVertex::Type::Background);

    float rawpdf;
    auto w_i = vertex.vertex.obj->m->sample(w_o, vertex.vertex.N, &rawpdf);
    float costheta = abs(DotProduct(vertex.vertex.N, w_i));
    float srpdf = SafeDivide(rawpdf, costheta);
    auto intersection = scene->Intersect(Ray(vertex.vertex.x, w_i), DotProduct(vertex.vertex.N, w_i) > 0.0f ? FaceCulling::CullBack : FaceCulling::CullFront);
    Vector3f bsdf = vertex.vertex.obj->m->evalGivenSample(w_o, w_i, vertex.vertex.N, false);

    BDPTPath::InternalPathVertex result;
    result.shadowed = false;
    result.vertex = intersection;
    result.alpha = SafeDivide(bsdf, srpdf);
    result.pdf = SrpdfToAreaPdf(srpdf, vertex.vertex, result.vertex);

    return result;
}


Vector3f BDPT(const Scene* scene, const Ray& ray, int& outBounces, Vector3f* emissionBuffer)
{
    outBounces = 0;
    auto lightPath = BDPTPath::BDPTPath(scene), camPath = BDPTPath::BDPTPath(scene);
    camPath.GenerateCameraPath(ray);
    lightPath.GenerateLightPath(scene->m_emissionObjects[0]);
    outBounces += camPath.count + lightPath.count;
    Vector3f result;
    for (int iCamPathLength = 1; iCamPathLength <= camPath.count; iCamPathLength++) {
        auto camSub = camPath.Sub(iCamPathLength);
        for (int iLightPathLength = 0; iLightPathLength <= lightPath.count; iLightPathLength++) {
            if (iCamPathLength + iLightPathLength < 2)
                continue;

            auto lightSub = lightPath.Sub(iLightPathLength);
            auto pathWeight = BDPTPath::PathWeight(lightSub, camSub);
            pathWeight = Vector3f::Max(pathWeight, 0.0f);

            if (camSub.count > 1) {
                result += pathWeight;
            }
            else {
                if (emissionBuffer != nullptr) {
                    auto light = lightSub[iLightPathLength - 1].Position();
                    auto cam = camSub[0].Position();
                    auto lightRayHitCamera = (light - cam).Normalized();
                    DrawToImage(Ray(light, lightRayHitCamera), emissionBuffer, pathWeight, scene->fov, scene->width, scene->height);
                }
            }
        }
    }
    return result;
}

Vector3f PathVertex::EvalBsdfOnSolidAngle(Vector3f dir) {
    assert(Type() != PTVertex::Type::Background);

    if (Type() == PTVertex::Type::Light) {
        return 1.0f;
    }
    else if (Type() == PTVertex::Type::Camera) {
        return 1.0f;
    }
    else {
        assert(index >= 1);
        return Material()->evalGivenSample((Pre().Position() - Position()).Normalized(), dir, Normal(), false);
    }
}

float PathVertex::EvalPdfOnSolidAngle(Vector3f dir) {
	assert(Type() != PTVertex::Type::Background);

	if (Type() == PTVertex::Type::Light) {
		return GetCosineWeightedPdf(Normal(), dir);
	}
	else if (Type() == PTVertex::Type::Camera) {
		return 1.0f;
	}
	else {
		float cosine = abs(DotProduct(dir, Normal()));
		if (cosine == 0.0f)
			return 0.0f;
		Vector3f wo = (Pre().Position() - Position()).Normalized();
		return Material()->pdf(
			wo,
			Normal(),
			dir) / cosine;
	}
}

float PathVertex::EvalPdfOnAreaSurface(const Scene* scene, PTVertex v) {
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

Vector3f PathVertex::Sample(float* pdf) {
	assert(Type() != PTVertex::Type::Camera);
	assert(Type() != PTVertex::Type::Background);


	if (Type() == PTVertex::Type::Light) {
		return GetCosineWeightedSample(Normal(), *pdf);
	}
	else {
		Vector3f w_o = (Pre().Position() - Position()).Normalized();
		return Material()->sample(w_o, Normal(), pdf);
	}
}
