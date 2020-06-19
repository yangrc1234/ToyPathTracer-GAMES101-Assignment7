cmake_minimum_required(VERSION 3.10)
project(RayTracing)

set(CMAKE_CXX_STANDARD 17)

add_executable(RayTracing main.cpp Object.hpp Vector.cpp Vector.hpp Sphere.hpp Sphere.cpp global.hpp Triangle.hpp Triangle.cpp Scene.cpp
        Scene.hpp BVH.cpp BVH.hpp Bounds3.hpp Ray.hpp Material.hpp Intersection.hpp
        Renderer.cpp Renderer.hpp Random.cpp stb_image_write.h Material.cpp global.cpp PathTracer.hpp PathTracer.cpp BDPT.hpp BDPT.cpp SampleHelperFunctions.hpp SampleHelperFunctions.cpp GGX.hpp PTVertex.hpp SceneRenderingHelper.hpp SceneRenderingHelper.cpp)
