//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include <cmath>
#include <cassert>

void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
    for (auto& t : objects) {
        if (t->hasEmit()) {
            m_emissionObjects.push_back(t);
        }
    }
}

Intersection Scene::intersect(const Ray &ray, FaceCulling culling ) const
{
    return this->bvh->Intersect(ray, culling);
}

bool Scene::shadowCheck(Vector3f lightCoords, Vector3f x) const
{
    auto lightDistanceSqr = dotProduct(lightCoords - x, lightCoords - x);
    //Shadow check.
    auto shadowInter = intersect(Ray(lightCoords, (x - lightCoords).normalized()));
    auto shadowDistanceSqr = dotProduct(shadowInter.coords - lightCoords, shadowInter.coords - lightCoords);
    if (shadowInter.happened && shadowDistanceSqr < lightDistanceSqr - 1.0f) {
        //Shadowed.
        return true;
    }
    return false;
}

inline float lerpf(float a, float b, float t){
    return (1.0f - t ) * a + t * b;
}

class DirectLightSampler {
public:
    DirectLightSampler(Object* emissionObject)
    {
        this->obj = emissionObject;
    }
    Object* obj;

    float pdf(Vector3f x, Vector3f w_o, Vector3f n, Vector3f w_i) {
        auto intersection = obj->getIntersection(Ray(x, w_i), FaceCulling::NoCull);
        if (!intersection.happened)
            return 0.0f;
        float lightDistanceSqr = dotProduct(intersection.coords - x, intersection.coords - x);
        float rawpdf = obj->pdf();
        float costhetap = dotProduct(intersection.normal, -w_i);
        if (costhetap == 0.0f)
            return 0.0f;
        return (double)rawpdf * lightDistanceSqr / abs(costhetap);   //When we generate sample, even back-facing lights are considered. So shouldn't cull those lights here(using a abs).
    }

    Vector3f sample(Vector3f x, Vector3f w_o, Vector3f n, float* pdf) {
        Intersection pos;
        obj->Sample(pos, *pdf);
        pos.m = obj->m;
        pos.happened = true;
        Vector3f w_i = (pos.coords - x);
        float lightDistanceSqr = dotProduct(w_i, w_i);
        w_i = w_i.normalized();
        float rawpdf = obj->pdf();
        float costhetap = dotProduct(pos.normal, -w_i);
        if (costhetap == 0.0f)
            *pdf = 0.0f;
        *pdf = (double)rawpdf * lightDistanceSqr / abs(costhetap);   //When we generate sample, even back-facing lights are considered. So shouldn't cull those lights here.
        return w_i;
    }
};

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int& outBounces) const
{
    outBounces = 0;
    Ray currentRay = ray;
    Vector3f throughput = 1.0f;
    Vector3f resultRadiance = 0.0f;
    bool lastBounceExplicitSampledLight = false;
    bool lastBounceFlipCulling = false;

    while (true) {
        if (throughput.x == 0.0f && throughput.y == 0.0f && throughput.z == 0.0f)
            break;
        auto intersection = intersect(currentRay, lastBounceFlipCulling ? FaceCulling::CullFront : FaceCulling::CullBack);

        if (!intersection.happened) {
            // std::cout << throughput<<std::endl;
            //resultRadiance += throughput * backgroundColor;
            break;
        }

        if (intersection.m->hasEmission()) {
            if (!lastBounceExplicitSampledLight) {
                resultRadiance += throughput * intersection.m->getEmission();
            }
        }

        Vector3f x = intersection.coords;
        Vector3f w_o = -currentRay.direction;
        Vector3f n = intersection.normal;
        auto mat = intersection.m;

        auto bsdfSampler = mat->GetSampler();
        float pdf_bsdf;
        Vector3f w_i_bsdf = bsdfSampler.sample(x, w_o, n, &pdf_bsdf);
        //return w_i_bsdf;
#if 1
        {
            lastBounceExplicitSampledLight = true;

            for (int iLight = 0; iLight < m_emissionObjects.size(); iLight++) {
                auto& light = m_emissionObjects[iLight];
                auto lightSampler = DirectLightSampler(light);
                float pdf_light_light, pdf_bsdf_light, pdf_light_bsdf;
                Vector3f w_i_light = lightSampler.sample(x, w_o, n, &pdf_light_light);
                pdf_light_bsdf = bsdfSampler.pdf(x, w_o, n, w_i_light); 
                pdf_bsdf_light = lightSampler.pdf(x, w_o, n, w_i_bsdf);

                Vector3f eval_result = 0;
                
                if (pdf_bsdf + pdf_bsdf_light > 0.0f) {
                    auto inte = light->getIntersection(Ray(x, w_i_bsdf), FaceCulling::CullBack);
                    
                    if (inte.happened && !shadowCheck(inte.coords, x)){
                        eval_result += mat->evalGivenSample(w_o, w_i_bsdf, n) / (EPSILON + pdf_bsdf + pdf_bsdf_light);
                    }
                }
                if (pdf_light_light + pdf_light_bsdf > 0.0f) {
                    auto inte = light->getIntersection(Ray(x, w_i_light), FaceCulling::CullBack);
                    if (!shadowCheck(inte.coords, x))
                        eval_result += mat->evalGivenSample(w_o, w_i_light, n) / (EPSILON + pdf_light_light + pdf_light_bsdf);
                }

                resultRadiance += throughput * eval_result * light->m->m_emission;
            }
        }
#endif

        Vector3f weight = 0;
        if (pdf_bsdf > 0.0f) {
            weight = mat->evalGivenSample(w_o, w_i_bsdf, n) / (EPSILON + pdf_bsdf);
        }

        currentRay = Ray(x, w_i_bsdf);
        if (dotProduct(n, w_i_bsdf) < 0.0f)
            lastBounceFlipCulling = true;
        else
            lastBounceFlipCulling = false;

        bool doRussianRoulette = outBounces > 4;
        if (!doRussianRoulette || get_random_float() < RussianRoulette) {
            throughput = throughput * weight
                / (doRussianRoulette ? RussianRoulette : 1.0f) ;
            outBounces += 1;
            continue;
        }
        else {
            break;
        }
    }
    return resultRadiance;
}