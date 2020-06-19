//
// Created by goksu on 2/25/20.
//
#pragma once

#include "Scene.hpp"

class Renderer
{
public:
    void Render(std::string outputFileName, const Scene& scene, int spp, int thread_count, bool bdpt);

private:
};
