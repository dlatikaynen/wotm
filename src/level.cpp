#include "inc/level.h"
#include "inc/levelscript.h"

#include <cairo.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
    cairo_surface_t* g_surface = nullptr;
    cairo_t* g_cr = nullptr;
    int g_w = 0;
    int g_h = 0;

    std::vector<DrawCmd> g_commands;
    LevelLighting        g_light;
    std::size_t          g_executed = 0;

    // commands per frame, load duration = total/this/fps
    // settable from script via gfx.commands_per_frame(n)
    int g_commandsPerFrame = 1;

    // single composite over the finished scene
    void apply_lighting(cairo_t* cr, int w, int h, const LevelLighting& L)
    {
        if (!L.hasAmbient && L.lights.empty())
        {
            return;
        }

        // build the lightmap on its own surface
        cairo_surface_t* lm = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        cairo_t* lc = cairo_create(lm);

        cairo_set_source_rgb(lc, L.ar, L.ag, L.ab);
        cairo_paint(lc);
        cairo_set_operator(lc, CAIRO_OPERATOR_ADD);
        
        for (const auto& li : L.lights)
        {
            cairo_pattern_t* p = cairo_pattern_create_radial(li.x, li.y, 0.0, li.x, li.y, li.radius);
            cairo_pattern_add_color_stop_rgba(
                p,
                0.0,
                li.r * li.intensity,
                li.g * li.intensity,
                li.b * li.intensity,
                1.0
            );

            cairo_pattern_add_color_stop_rgba(p, 1.0, 0.0, 0.0, 0.0, 1.0);
            cairo_set_source(lc, p);
            cairo_paint(lc);
            cairo_pattern_destroy(p);
        }

        cairo_destroy(lc);
        cairo_set_operator(cr, CAIRO_OPERATOR_MULTIPLY);
        cairo_set_source_surface(cr, lm, 0.0, 0.0);
        cairo_paint(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_surface_destroy(lm);
    }

    void blit_to_texture(Gamestate* state)
    {
        cairo_surface_flush(g_surface);
        SDL_UpdateTexture(
            state->texArena,
            nullptr, //rect
            cairo_image_surface_get_data(g_surface),
            cairo_image_surface_get_stride(g_surface)
        );
    }
}

void level_begin(SDL_Renderer* r, Gamestate* state, int w, int h)
{
    if (g_cr != nullptr)
    {
        cairo_destroy(g_cr);
        g_cr = nullptr;
    }

    if (g_surface != nullptr)
    {
        cairo_surface_destroy(g_surface);
        g_surface = nullptr;
    }

    g_w = w;
    g_h = h;

    g_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    g_cr = cairo_create(g_surface);

    // opaque so the surface is initially black
    cairo_set_source_rgb(g_cr, 0.0, 0.0, 0.0);
    cairo_paint(g_cr);

    // sink for the lua draw commands
    std::string err;

    g_commandsPerFrame = 1;
    if (!run_level_script("data/level1.lua", g_commands, g_light, g_commandsPerFrame, err))
    {
        std::cerr << "[level] script failed (" << err << ")\n";        
        g_light = LevelLighting{};
    }

    g_executed = 0;
    if (state->texArena != nullptr)
    {
        SDL_DestroyTexture(state->texArena);
    }

    state->texArena = SDL_CreateTexture(
        r,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        w,
        h
    );

    blit_to_texture(state);
}

float level_advance(Gamestate* state)
{
    if (g_cr == nullptr || state->texArena == nullptr || g_commands.empty())
    {
        return 1.0f;
    }

    const std::size_t budget = g_commandsPerFrame > 0 ? static_cast<std::size_t>(g_commandsPerFrame) : 1;
    const std::size_t end = std::min(g_executed + budget, g_commands.size());

    for (; g_executed < end; ++g_executed)
    {
        g_commands[g_executed](g_cr);
    }

    blit_to_texture(state);

    return static_cast<float>(g_executed) / static_cast<float>(g_commands.size());
}

void level_complete(Gamestate* state)
{
    // run any remaining commands, then apply the global lighting
    while (g_executed < g_commands.size())
    {
        g_commands[g_executed++](g_cr);
    }

    apply_lighting(g_cr, g_w, g_h, g_light);
    blit_to_texture(state);
    if (g_cr != nullptr)
    {
        cairo_destroy(g_cr);
        g_cr = nullptr;
    }

    if (g_surface != nullptr)
    {
        cairo_surface_destroy(g_surface);
        g_surface = nullptr;
    }

    g_commands.clear();
    g_executed = 0;
}

void level_step(void* userData)
{
    const auto& state = static_cast<Gamestate*>(userData);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (state->texArena != nullptr)
    {
        SDL_RenderTexture(renderer, state->texArena, nullptr, nullptr);
    }

    SDL_RenderPresent(renderer);
}
