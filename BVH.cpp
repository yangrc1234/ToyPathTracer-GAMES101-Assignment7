#include <algorithm>
#include <cassert>
#include "BVH.hpp"

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
      primitives(std::move(p))
{
    time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;
#ifdef BVH_NODE_ARRAY_LAYOUT
    recursiveBuild(primitives);
#else
    root = recursiveBuild(primitives);
#endif
    time(&stop);
    double diff = difftime(stop, start);
    int hrs = (int)diff / 3600;
    int mins = ((int)diff / 60) - (hrs * 60);
    int secs = (int)diff - (hrs * 3600) - (mins * 60);

    printf(
        "\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
        hrs, mins, secs);
}

BVHNodeIndex BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHNodeIndex nodeIndex = allocateNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        GN(nodeIndex).bounds = objects[0]->getBounds();
        GN(nodeIndex).object = objects[0];
        GN(nodeIndex).left = BVHNodeNull;
        GN(nodeIndex).right = BVHNodeNull;
        GN(nodeIndex).area = objects[0]->getArea();
        return nodeIndex;
    }
    else if (objects.size() == 2) {
        GN(nodeIndex).left = recursiveBuild(std::vector{objects[0]});
        GN(nodeIndex).right = recursiveBuild(std::vector{objects[1]});

        GN(nodeIndex).bounds = Union(GN(GN(nodeIndex).left).bounds, GN(GN(nodeIndex).right).bounds);
        GN(nodeIndex).area = GN(GN(nodeIndex).left).area + GN(GN(nodeIndex).right).area;
        return nodeIndex;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        auto beginning = objects.begin();
        auto middling = objects.begin() + (objects.size() / 2);
        auto ending = objects.end();

        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        GN(nodeIndex).left = recursiveBuild(leftshapes);
        GN(nodeIndex).right = recursiveBuild(rightshapes);

        GN(nodeIndex).bounds = Union(GN(GN(nodeIndex).left).bounds, GN(GN(nodeIndex).right).bounds);
        GN(nodeIndex).area = GN(GN(nodeIndex).left).area + GN(GN(nodeIndex).right).area;
    }

    return nodeIndex;
}

const int intersectionStackSize = 64;

Intersection BVHAccel::Intersect(const Ray& ray, FaceCulling culling) const
{
    Intersection insect;
    BVHNodeIndex intersectionStack[intersectionStackSize];
    int stackOffset = 0;

#ifdef BVH_NODE_ARRAY_LAYOUT
    if (nodes.size() == 0)
        return insect;
    intersectionStack[stackOffset++] = 0;
#else
    if (root == BVHNodeNull)
        return insect;
    intersectionStack[stackOffset++] = root;
#endif


    while (stackOffset != 0) {
        auto front = intersectionStack[--stackOffset];
        const BVHBuildNode& node = GN(front);

        if (!node.bounds.IntersectP(ray, ray.direction_inv)){
            continue;
        }

        if (node.object != nullptr){
            auto t = node.object->getIntersection(ray, culling);
            if (t.happened) {
                if (!insect.happened || insect.distance > t.distance) {
                    insect = t;
                }
            }
        } else {
            if (stackOffset + 2 < intersectionStackSize) {
                intersectionStack[stackOffset++] = node.left;
                intersectionStack[stackOffset++] = node.right;
            }
        }
    }
    return insect;
}

void BVHAccel::getSample(BVHNodeIndex index, float p, Intersection &pos){
    auto& node = GN(index);

    if(node.left == BVHNodeNull || node.right == BVHNodeNull){
        node.object->Sample(pos);
        return;
    }
    if(p < GN(node.left).area) getSample(node.left, p, pos);
    else getSample(node.right, p - GN(node.left).area, pos);
}

void BVHAccel::Sample(Intersection &pos){
    float p = std::sqrt(get_random_float()) * GN(Root()).area;
    getSample(Root(), p, pos);
}

BVHNodeIndex BVHAccel::allocateNode()
{
#ifdef BVH_NODE_ARRAY_LAYOUT
    nodes.push_back(BVHBuildNode());
    return nodes.size() - 1;
#else
    return new BVHBuildNode();
#endif
}
