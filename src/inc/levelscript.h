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

struct LevelLayer
{
    std::vector<DrawCmd> cmds;
    LevelLighting light;
    int arenaW = 0;
    int arenaH = 0;
    int scrollX = 0;
    int scrollY = 0;
};

bool run_level_script(
    const std::string& path,
    int viewportW,
    int viewportH,
    LevelLayer& background,
    LevelLayer& arena,
    LevelLayer& foreground,
    int& commandsPerFrame,
    std::string& err
);
