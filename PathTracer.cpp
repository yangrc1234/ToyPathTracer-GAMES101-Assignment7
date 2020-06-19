#include "PathTracer.hpp"
#include "Object.hpp"

#define RussianRoulette 0.8f

class DirectLightSampler {
public:
    DirectLightSampler(Object* emissionObject)
    {
        this->obj = emissionObject;
    }
    Object* obj;

    float pdf(Vector3f x, Vector3f w_o, Vector3f n, Vector3f w_i) {
        auto intersection = obj->GetIntersection(Ray(x, w_i), FaceCulling::NoCull);
        if (!intersection.happened)
            return 0.0f;
        float lightDistanceSqr = DotProduct(intersection.coords - x, intersection.coords - x);
        float rawpdf = obj->pdf();
        float costhetap = DotProduct(intersection.normal, -w_i);
        if (costhetap == 0.0f)
            return 0.0f;
        return (double)rawpdf * lightDistanceSqr / abs(costhetap);   //When we generate sample, even back-facing lights are considered. So shouldn't cull those lights here(using a abs).
    }

    Vector3f sample(Vector3f x, Vector3f w_o, Vector3f n, float* pdf) {
        Intersection pos;
        obj->Sample(pos);
        pos.m = obj->m;
        pos.happened = true;
        Vector3f w_i = (pos.coords - x);
        float lightDistanceSqr = DotProduct(w_i, w_i);
        w_i = w_i.Normalized();
        float rawpdf = obj->pdf();
        float costhetap = DotProduct(pos.normal, -w_i);
        if (costhetap == 0.0f)
            *pdf = 0.0f;
        *pdf = (double)rawpdf * lightDistanceSqr / abs(costhetap);   //When we generate sample, even back-facing lights are considered. So shouldn't cull those lights here.
        return w_i;
    }
};

// Implementation of Path Tracing
Vector3f PathTrace(const Scene* scene, const Ray& ray, int& outBounces)
{
    outBounces = 0;
    Ray currentRay = ray;
    Vector3f alpha = 1.0f;
    Vector3f resultRadiance = 0.0f;
    bool lastBounceExplicitSampledLight = false;
    bool lastBounceFlipCulling = false;

    while (true) {
        if (alpha.x == 0.0f && alpha.y == 0.0f && alpha.z == 0.0f)
            break;
        auto intersection = scene->Intersect(currentRay, lastBounceFlipCulling ? FaceCulling::CullFront : FaceCulling::CullBack);

        if (intersection.type == PTVertex::Type::Background) {
            // std::cout << throughput<<std::endl;
            //resultRadiance += throughput * backgroundColor;
            break;
        }

        if (intersection.obj->m->hasEmission()) {
            if (!lastBounceExplicitSampledLight) {
                resultRadiance += alpha * intersection.obj->m->GetEmission();
            }
        }

        Vector3f x = intersection.x;
        Vector3f w_o = -currentRay.direction;
        Vector3f n = intersection.N;
        auto mat = intersection.obj->m;

        float pdf_bsdf;
        Vector3f w_i_bsdf = mat->sample(w_o, n, &pdf_bsdf);
        //return w_i_bsdf;
#if 1
        {
            lastBounceExplicitSampledLight = true;

            for (int iLight = 0; iLight < scene->m_emissionObjects.size(); iLight++) {
                auto& light = scene->m_emissionObjects[iLight];
                auto lightSampler = DirectLightSampler(light);
                float pdf_light_light, pdf_bsdf_light, pdf_light_bsdf;
                Vector3f w_i_light = lightSampler.sample(x, w_o, n, &pdf_light_light);
                pdf_light_bsdf = mat->pdf(w_o, n, w_i_light);
                pdf_bsdf_light = lightSampler.pdf(x, w_o, n, w_i_bsdf);

                Vector3f eval_result = 0;

                if (pdf_bsdf + pdf_bsdf_light > 0.0f) {
                    auto inte = light->GetIntersection(Ray(x, w_i_bsdf), FaceCulling::CullBack);

                    if (inte.happened && !scene->ShadowCheck(inte.coords, x)) {
                        eval_result += mat->evalGivenSample(w_o, w_i_bsdf, n) / (EPSILON + pdf_bsdf + pdf_bsdf_light);
                    }
                }
                if (pdf_light_light + pdf_light_bsdf > 0.0f) {
                    auto inte = light->GetIntersection(Ray(x, w_i_light), FaceCulling::CullBack);
                    if (!scene->ShadowCheck(inte.coords, x))
                        eval_result += mat->evalGivenSample(w_o, w_i_light, n) / (EPSILON + pdf_light_light + pdf_light_bsdf);
                }

                resultRadiance += alpha * eval_result * light->m->m_emission;
            }
        }
#endif

        Vector3f weight = 0;
        if (pdf_bsdf > 0.0f) {
            weight = mat->evalGivenSample(w_o, w_i_bsdf, n) / (EPSILON + pdf_bsdf);
        }

        currentRay = Ray(x, w_i_bsdf);
        if (DotProduct(n, w_i_bsdf) < 0.0f)
            lastBounceFlipCulling = true;
        else
            lastBounceFlipCulling = false;

        bool doRussianRoulette = outBounces > 4;
        if (!doRussianRoulette || GetRandomFloat() < RussianRoulette) {
            alpha = alpha * weight
                / (doRussianRoulette ? RussianRoulette : 1.0f);
            outBounces += 1;
            continue;
        }
        else {
            break;
        }
    }
    return resultRadiance;
}