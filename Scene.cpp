//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include <cmath>
#include <cassert>
#include "BDPT.hpp"
#include "Renderer.hpp"

void Scene::BuildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
    for (auto& t : objects) {
        if (t->hasEmit()) {
            m_emissionObjects.push_back(t);
        }
    }
}

PTVertex Scene::Intersect(const Ray &ray, FaceCulling culling) const
{
    auto t = this->bvh->Intersect(ray, culling);
    PTVertex r;
    if (t.happened) {
        r.obj = t.obj;
        r.N = t.normal;
        r.type = PTVertex::Type::Intermediate;
        r.x = t.coords;
    }
    else {
        r.type = PTVertex::Type::Background;
    }
    return r;
}

bool Scene::ShadowCheck(Vector3f lightCoords, Vector3f x, FaceCulling culling) const
{
    auto lightDistanceSqr = DotProduct(lightCoords - x, lightCoords - x);
    //Shadow check.
    auto shadowInter = Intersect(Ray(lightCoords, (x - lightCoords).Normalized()), culling);
    auto shadowDistanceSqr = DotProduct(shadowInter.x - lightCoords, shadowInter.x - lightCoords);
    if (shadowInter.type != PTVertex::Type::Background && shadowDistanceSqr < lightDistanceSqr - 1.0f) {
        //Shadowed.
        return true;
    }
    return false;
}

bool Scene::ShadowCheck(const PTVertex& v1, const PTVertex& v2) const {
	/*
	A more optimized shadow check.
	The input must be PTVertex, so the function could know about facing, normal etc.
	With facing and normal we could fast path some conditions quickly without accesing BVH.  
	*/
	Vector3f atob = v2.x - v1.x;

	if (v1.obj != nullptr && v2.obj != v1.obj && v1.obj->m->getType() == MaterialType::Transparent) {    //v1 and v2 on the same transmittance object.
																													//Transmittance material.
		if (DotProduct(atob, v1.N) < 0.0f) {
			//v2 is on backside of v1 surface. flip culling.
			return ShadowCheck(v1.x, v2.x, FaceCulling::CullFront);
		}
		else {
			//Normal shadow checking.
			return ShadowCheck(v1.x, v2.x);
		}
	}
	else {

		if (v1.obj != nullptr && DotProduct(atob, v1.N) < 0.0f) {
			//Fast path, if v1 is object, use its normal facing to quick detect occlusion.
			return false;
		}
		if (v2.obj != nullptr && DotProduct(-atob, v2.N) < 0.0f) {
			//Same as above.
			return false;
		}

		//Slowest detection.
		return ShadowCheck(v1.x, v2.x);
	}
}


