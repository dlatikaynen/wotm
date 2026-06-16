#include "inc/level.h"

#include <cairo.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace
{
    // each of these is emitted by a Lua call
    // here a placeholder generator fills the list
    using DrawCmd = std::function<void(cairo_t*)>;
    cairo_surface_t* g_surface = nullptr;
    cairo_t* g_cr = nullptr;
    int g_w = 0;
    int g_h = 0;

    std::vector<DrawCmd> g_commands;
    std::size_t          g_executed = 0;

    // how many commands to run per frame; load duration = total / this / fps
    constexpr std::size_t COMMANDS_PER_FRAME = 1;

    double hill_y(double x, int h)
    {
        return h * 0.62
            + std::sin(x * 0.025) * 18.0
            + std::sin(x * 0.011 + 1.3) * 28.0;
    }

    // Placeholder command stream. Stands in for a Lua scene walk: commands run in
    // paint order and draw all over the surface, some overlapping. Step 3 replaces
    // this generator with one fed by the script.
    void build_commands(int w, int h)
    {
        g_commands.clear();
        g_executed = 0;

        // deterministic PRNG so the scene is reproducible frame to frame / run to run
        std::uint32_t rng = 0x1234567u;
        auto next  = [&] { rng = rng * 1664525u + 1013904223u; return rng; };
        auto frand = [&] { return static_cast<double>(next() >> 8) * (1.0 / 16777216.0); };

        // sky gradient (one big command)
        g_commands.push_back([h](cairo_t* cr) {
            cairo_pattern_t* sky = cairo_pattern_create_linear(0, 0, 0, h);
            cairo_pattern_add_color_stop_rgb(sky, 0.0, 0.05, 0.07, 0.16);
            cairo_pattern_add_color_stop_rgb(sky, 1.0, 0.20, 0.16, 0.28);
            cairo_set_source(cr, sky);
            cairo_paint(cr);
            cairo_pattern_destroy(sky);
        });

        // stars scattered across the sky
        for (int i = 0; i < 120; ++i)
        {
            const double x = frand() * w;
            const double y = frand() * h * 0.55;
            const double r = 0.4 + frand() * 1.1;
            const double a = 0.3 + frand() * 0.6;
            g_commands.push_back([x, y, r, a](cairo_t* cr) {
                cairo_set_source_rgba(cr, 1.0, 0.97, 0.85, a);
                cairo_arc(cr, x, y, r, 0.0, 2.0 * M_PI);
                cairo_fill(cr);
            });
        }

        // terrain silhouette
        g_commands.push_back([w, h](cairo_t* cr) {
            cairo_new_path(cr);
            cairo_move_to(cr, 0, h);
            for (int x = 0; x <= w; ++x)
            {
                cairo_line_to(cr, x, hill_y(x, h));
            }
            cairo_line_to(cr, w, h);
            cairo_close_path(cr);
            cairo_set_source_rgb(cr, 0.10, 0.30, 0.18);
            cairo_fill(cr);
        });

        // trees standing on the terrain
        for (int i = 0; i < 40; ++i)
        {
            const double x  = frand() * w;
            const double gy = hill_y(x, h);
            const double th = 10.0 + frand() * 16.0; // trunk height
            g_commands.push_back([x, gy, th](cairo_t* cr) {
                cairo_set_source_rgb(cr, 0.22, 0.14, 0.08);
                cairo_rectangle(cr, x - 1.0, gy - th, 2.0, th);
                cairo_fill(cr);
                cairo_set_source_rgb(cr, 0.08, 0.24, 0.12);
                cairo_arc(cr, x, gy - th, 5.0, 0.0, 2.0 * M_PI);
                cairo_fill(cr);
            });
        }

        // rocks near the ground
        for (int i = 0; i < 30; ++i)
        {
            const double x  = frand() * w;
            const double gy = hill_y(x, h) + frand() * 8.0;
            const double rw = 2.0 + frand() * 5.0;

            g_commands.push_back([x, gy, rw](cairo_t* cr) {
                cairo_set_source_rgb(cr, 0.18, 0.18, 0.20);
                cairo_save(cr);
                cairo_translate(cr, x, gy);
                cairo_scale(cr, 1.0, 0.6);
                cairo_arc(cr, 0.0, 0.0, rw, 0.0, 2.0 * M_PI);
                cairo_restore(cr);
                cairo_fill(cr);
            });
        }
    }

    // Lighting pass: a single composite over the finished scene, not a streamed
    // command. Multiply by a radial lightmap so the centre stays lit and the
    // edges fall into shadow.
    void finalize_lighting(cairo_t* cr, int w, int h)
    {
        cairo_pattern_t* light = cairo_pattern_create_radial(
            w * 0.5, h * 0.45, h * 0.10,
            w * 0.5, h * 0.45, w * 0.70);

        cairo_pattern_add_color_stop_rgb(light, 0.0, 1.00, 0.97, 0.90);
        cairo_pattern_add_color_stop_rgb(light, 1.0, 0.32, 0.32, 0.42);
        cairo_set_operator(cr, CAIRO_OPERATOR_MULTIPLY);
        cairo_set_source(cr, light);
        cairo_paint(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_pattern_destroy(light);
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

    // opaque so the not-yet-drawn surface reads as black
    cairo_set_source_rgb(g_cr, 0.0, 0.0, 0.0);
    cairo_paint(g_cr);
    build_commands(w, h);
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

    const std::size_t end = std::min(g_executed + COMMANDS_PER_FRAME, g_commands.size());

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

    finalize_lighting(g_cr, g_w, g_h);
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
