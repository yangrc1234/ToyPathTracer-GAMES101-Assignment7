//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_BVH_H
#define RAYTRACING_BVH_H

#include <atomic>
#include <vector>
#include <memory>
#include <ctime>
#include "Object.hpp"
#include "Ray.hpp"
#include "Bounds3.hpp"
#include "Intersection.hpp"
#include "Vector.hpp"

struct BVHBuildNode;
// BVHAccel Forward Declarations
struct BVHPrimitiveInfo;

#define BVH_NODE_ARRAY_LAYOUT

#ifdef BVH_NODE_ARRAY_LAYOUT
typedef int BVHNodeIndex;
const BVHNodeIndex BVHNodeNull = -1;
#else
typedef BVHBuildNode* BVHNodeIndex;
const BVHNodeIndex BVHNodeNull = nullptr;
#endif

class BVHAccel {

public:
    // BVHAccel Public Types
    enum class SplitMethod { NAIVE, SAH };

    // BVHAccel Public Methods
    BVHAccel(std::vector<Object*> p, int maxPrimsInNode = 1, SplitMethod splitMethod = SplitMethod::NAIVE);
    ~BVHAccel();

    Intersection Intersect(const Ray &ray, FaceCulling cull) const;

    // BVHAccel Private Methods
    BVHNodeIndex recursiveBuild(std::vector<Object*>objects);

    // BVHAccel Private Data
    const int maxPrimsInNode;
    const SplitMethod splitMethod;
    std::vector<Object*> primitives;

#ifdef BVH_NODE_ARRAY_LAYOUT
    std::vector<BVHBuildNode> nodes;
#else
    BVHNodeIndex root;
#endif

    BVHNodeIndex Root() {

#ifdef BVH_NODE_ARRAY_LAYOUT
        return 0;
#else
        return root;
#endif
    }

    void getSample(BVHNodeIndex index, float p, Intersection &pos);
    void Sample(Intersection &pos);

    BVHNodeIndex allocateNode();

    inline BVHBuildNode& GN(BVHNodeIndex index) {
#ifdef BVH_NODE_ARRAY_LAYOUT
        return nodes[index];
#else
        return *index;
#endif
    }

    inline const BVHBuildNode& GN(BVHNodeIndex index) const {
#ifdef BVH_NODE_ARRAY_LAYOUT
        return nodes[index];
#else
        return *index;
#endif
    }
};

struct BVHBuildNode {
    Bounds3 bounds;
    BVHNodeIndex left;
    BVHNodeIndex right;
    Object* object;
    float area;

public:
    int splitAxis=0, firstPrimOffset=0, nPrimitives=0;
    // BVHBuildNode Public Methods
    BVHBuildNode(){
        bounds = Bounds3();
        left = BVHNodeNull; right = BVHNodeNull;
        object = nullptr;
    }
};




#endif //RAYTRACING_BVH_H
