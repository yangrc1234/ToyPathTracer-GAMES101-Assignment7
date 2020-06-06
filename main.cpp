#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Sphere.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>
#include <sstream>

template<typename T> 
T tryParseArg(int argc, char** argv, const char* argName, const T& defaultValue){
    for (size_t i = 0; i < argc; i++)
    {
        if (std::string(argv[i]) == argName && i + 1 != argc){
            T tempValue;
            std::stringstream ss(argv[i + 1]);
            ss >> tempValue;
            return tempValue;
        }
    }
    return defaultValue;
}

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{
    int spp = tryParseArg(argc, argv, "-spp", 4);
    int thread = tryParseArg(argc, argv, "-j", 4);
    // Change the definition here to change resolution
    Scene scene(784, 784);
    scene.eyePos = Vector3f(278, 278, -800);
    scene.backgroundColor = Vector3f(0.1f, 0.1f, 0.2f);
    Material* red = new Material(Dieletric, Vector3f(0.0f));
    red->Kd = Vector3f(0.63f, 0.065f, 0.05f);
    Material* green = new Material(Dieletric, Vector3f(0.0f));
    green->Kd = Vector3f(0.14f, 0.45f, 0.091f);
    Material* white = new Material(Dieletric, Vector3f(0.0f));
    white->Kd = Vector3f(0.725f, 0.71f, 0.68f);
    white->SetSmoothness(.1f);
    Material* light = new Material(Dieletric, (8.0f * Vector3f(0.747f+0.058f, 0.747f+0.258f, 0.747f) + 15.6f * Vector3f(0.740f+0.287f,0.740f+0.160f,0.740f) + 18.4f *Vector3f(0.737f+0.642f,0.737f+0.159f,0.737f)));
    light->Kd = Vector3f(0.65f);
    Material* metal = new Material(Metal);
    metal->SetSmoothness(1.0f);
    Material* smoothmetal = new Material(Metal);
    smoothmetal->SetSmoothness(0.7f);
    Material* steel = new Material(Metal);
    steel->ior_m = Vector3f(2.8653, 2.8889, 2.4006);
    steel->ior_m_k = Vector3f(3.1820, 2.9164, 2.6773);

    Material* silver = new Material(Metal);
    silver->ior_m = Vector3f(0.041000f, 0.53285f, 0.049317f);
    silver->ior_m_k = Vector3f(4.8025f, 3.4101f, 2.8545f);
    silver->SetSmoothness(0.9f);

    Material* copper = new Material(Metal);
    copper->ior_m = Vector3f(0.211f, 1.2174f, 1.2493);
    copper->ior_m_k = Vector3f(4.1592f, 2.5978f, 2.4771);
    copper->SetSmoothness(0.7f);

    Material* mglassBall = new Material(TransmittanceDieletric);
    mglassBall->ior_d = 1.5f;
    mglassBall->SetSmoothness(.9f);

    MeshTriangle floor("../models/cornellbox/floor.obj", white);
    MeshTriangle shortbox("../models/cornellbox/shortbox.obj", white);
    MeshTriangle tallbox("../models/cornellbox/tallbox.obj", white);
    MeshTriangle left("../models/cornellbox/left.obj", red);
    MeshTriangle right("../models/cornellbox/right.obj", green);
    MeshTriangle light_("../models/cornellbox/light.obj", light);

    Sphere glassBall(
        Vector3f(270.0f, 278.0f, 200.0f)
        , 50.0f, mglassBall);
    
    scene.Add(&floor);
    scene.Add(&shortbox);
    scene.Add(&tallbox);
    scene.Add(&left);
    scene.Add(&right);
    scene.Add(&light_);
    scene.Add(&glassBall);
    scene.buildBVH();
#if _DEBUG
    auto test = refract(Vector3f(-1.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.5f);
    auto test2 = refract(Vector3f(-1.0f, -1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.5f);
    auto test5 = refract(Vector3f(-10.0f, -1.0f, 0.0f).normalized(), Vector3f(0.0f, 1.0f, 0.0f), 1.5f);
    Vector3f t2;
    auto test3 = reflect(Vector3f(-1.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f));

    auto test4 = -(1.5f * Vector3f(-1.0f, -1.0f, 0.0f).normalized() + Vector3f(1.5f, 1.0f, 0.0f).normalized());

    int t;
    Vector3f debugPixel = Vector3f(234, 357, 0);
    Vector3f debugPixelUV = (debugPixel / Vector3f(scene.width, scene.height, 1.0f) - 0.5f) * 2.0f;

    float scale = CalculateScale(scene.fov);
    auto debugRay = PixelPosToRay(debugPixel.x, debugPixel.y, scene.width, scene.height, scale);
    reset_random(234 + 357 * scene.height);
    scene.castRay(Ray(scene.eyePos, debugRay), t);
#endif
    Renderer r;
    r.Render(scene, spp, thread);


    return 0;
}