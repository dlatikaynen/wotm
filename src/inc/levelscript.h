#pragma once

#include <cairo.h>
#include <functional>
#include <string>
#include <vector>

// recorded drawing primitive, replayed onto the level surface
using DrawCmd = std::function<void(cairo_t*)>;

// lighting from the script
struct LevelLighting
{
    struct Light
    {
        double x, y, radius;
        double r, g, b;
        double intensity;
    };

    bool hasAmbient = false;
    double ar = 0.0, ag = 0.0, ab = 0.0;
    std::vector<Light> lights;
};

bool run_level_script(
    const std::string& path,
    int viewportW,
    int viewportH,
    std::vector<DrawCmd>& out,
    LevelLighting& light,
    int& commandsPerFrame,
    int& arenaW,
    int& arenaH,
    int& scrollX,
    int& scrollY,
    std::string& err
);
