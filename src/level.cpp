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
    struct LayerRT
    {
        cairo_surface_t* surface = nullptr;
        cairo_t* cr = nullptr;
        int w = 0;
        int h = 0;

        std::vector<DrawCmd> cmds;
        LevelLighting light;
        std::size_t executed = 0;
        bool lit = false;
    };

    LayerRT g_bg;
    LayerRT g_arena;
    LayerRT g_fg;
    int g_commandsPerFrame = 1;

    void apply_lighting(LayerRT& layer)
    {
        const LevelLighting& L = layer.light;

        if (!L.hasAmbient && L.lights.empty())
        {
            return;
        }

        const int w = layer.w;
        const int h = layer.h;

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
        cairo_surface_flush(lm);
        cairo_surface_flush(layer.surface);

        const unsigned char* lp = cairo_image_surface_get_data(lm);
        const int lstride = cairo_image_surface_get_stride(lm);
        unsigned char* dpix = cairo_image_surface_get_data(layer.surface);
        const int dstride = cairo_image_surface_get_stride(layer.surface);

        for (int y = 0; y < h; ++y)
        {
            const unsigned char* lrow = lp + y * lstride;
            unsigned char* drow = dpix + y * dstride;

            for (int x = 0; x < w; ++x)
            {
                const unsigned char* l = lrow + x * 4;
                unsigned char* d = drow + x * 4;

                d[0] = static_cast<unsigned char>(d[0] * l[0] / 255);
                d[1] = static_cast<unsigned char>(d[1] * l[1] / 255);
                d[2] = static_cast<unsigned char>(d[2] * l[2] / 255);
            }
        }

        cairo_surface_mark_dirty(layer.surface);
        cairo_surface_destroy(lm);
    }

    void blit_layer(cairo_surface_t* surface, SDL_Texture* tex)
    {
        if (surface == nullptr || tex == nullptr)
        {
            return;
        }

        cairo_surface_flush(surface);
        SDL_UpdateTexture(
            tex,
            nullptr, //rect
            cairo_image_surface_get_data(surface),
            cairo_image_surface_get_stride(surface)
        );
    }

    void destroy_layer(LayerRT& layer)
    {
        if (layer.cr != nullptr)
        {
            cairo_destroy(layer.cr);
            layer.cr = nullptr;
        }

        if (layer.surface != nullptr)
        {
            cairo_surface_destroy(layer.surface);
            layer.surface = nullptr;
        }

        layer.cmds.clear();
        layer.light = LevelLighting{};
        layer.executed = 0;
        layer.lit = false;
        layer.w = 0;
        layer.h = 0;
    }

    void init_layer(LayerRT& layer, LevelLayer&& src, bool opaque)
    {
        layer.w = src.arenaW;
        layer.h = src.arenaH;
        layer.cmds = std::move(src.cmds);
        layer.light = std::move(src.light);
        layer.executed = 0;
        layer.lit = false;
        layer.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, layer.w, layer.h);
        layer.cr = cairo_create(layer.surface);

        if (opaque)
        {
            cairo_set_source_rgb(layer.cr, 0.0, 0.0, 0.0);
            cairo_paint(layer.cr);
        }
    }

    SDL_Texture* make_texture(SDL_Renderer* r, SDL_Texture* old, const LayerRT& layer)
    {
        if (old != nullptr)
        {
            SDL_DestroyTexture(old);
        }

        SDL_Texture* tex = SDL_CreateTexture(
            r,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            layer.w,
            layer.h
        );

        if (tex != nullptr)
        {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        }

        return tex;
    }

    std::size_t advance_layer(LayerRT& layer, std::size_t budget)
    {
        const std::size_t end = std::min(layer.executed + budget, layer.cmds.size());
        const std::size_t ran = end - layer.executed;

        for (; layer.executed < end; ++layer.executed)
        {
            layer.cmds[layer.executed](layer.cr);
        }

        return budget - ran;
    }

    void finish_layer(LayerRT& layer)
    {
        while (layer.executed < layer.cmds.size())
        {
            layer.cmds[layer.executed++](layer.cr);
        }

        if (!layer.lit)
        {
            apply_lighting(layer);
            layer.lit = true;
        }
    }

    std::size_t total_commands()
    {
        return g_bg.cmds.size() + g_arena.cmds.size() + g_fg.cmds.size();
    }

    std::size_t executed_commands()
    {
        return g_bg.executed + g_arena.executed + g_fg.executed;
    }
}

void level_begin(SDL_Renderer* r, Gamestate* state, int w, int h)
{
    LevelLayer background, arena, foreground;
    std::string err;

    destroy_layer(g_bg);
    destroy_layer(g_arena);
    destroy_layer(g_fg);
    g_commandsPerFrame = 1;
    if (!run_level_script(
        "data/level1.lua",
        w, h,
        background,
        arena,
        foreground,
        g_commandsPerFrame,
        err
    )) {
        std::cerr << "Level " << state->enteringLevel << " script failed: " << err << "\n";
        background = LevelLayer{};
        background.arenaW = w;
        background.arenaH = h;
        arena = LevelLayer{};
        arena.arenaW = w;
        arena.arenaH = h;
        foreground = LevelLayer{};
        foreground.arenaW = w;
        foreground.arenaH = h;
    }

    state->bgScrollX = static_cast<float>(background.scrollX);
    state->bgScrollY = static_cast<float>(background.scrollY);
    state->scrollX  = static_cast<float>(arena.scrollX);
    state->scrollY = static_cast<float>(arena.scrollY);
    state->fgScrollX = static_cast<float>(foreground.scrollX);
    state->fgScrollY = static_cast<float>(foreground.scrollY);

    init_layer(g_bg, std::move(background), true);
    init_layer(g_arena, std::move(arena), false);
    init_layer(g_fg, std::move(foreground), false);

    state->texBackground = make_texture(r, state->texBackground, g_bg);
    state->texArena = make_texture(r, state->texArena, g_arena);
    state->texForeground = make_texture(r, state->texForeground, g_fg);

    blit_layer(g_bg.surface, state->texBackground);
    blit_layer(g_arena.surface, state->texArena);
    blit_layer(g_fg.surface, state->texForeground);
}

float level_advance(Gamestate* state)
{
    if (g_bg.cr == nullptr || state->texArena == nullptr)
    {
        return 1.0f;
    }

    const std::size_t total = total_commands();

    if (total == 0)
    {
        return 1.0f;
    }

    std::size_t budget = g_commandsPerFrame > 0 
        ? static_cast<std::size_t>(g_commandsPerFrame)
        : 1;

    budget = advance_layer(g_bg, budget);
    budget = advance_layer(g_arena, budget);
    budget = advance_layer(g_fg, budget);

    blit_layer(g_bg.surface, state->texBackground);
    blit_layer(g_arena.surface, state->texArena);
    blit_layer(g_fg.surface, state->texForeground);

    return static_cast<float>(executed_commands()) / static_cast<float>(total);
}

void level_complete(Gamestate* state)
{
    finish_layer(g_bg);
    finish_layer(g_arena);
    finish_layer(g_fg);

    blit_layer(g_bg.surface, state->texBackground);
    blit_layer(g_arena.surface, state->texArena);
    blit_layer(g_fg.surface, state->texForeground);

    destroy_layer(g_bg);
    destroy_layer(g_arena);
    destroy_layer(g_fg);
}

void level_step(void* userData)
{
    const auto& state = static_cast<Gamestate*>(userData);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const float vw = static_cast<float>(WINDOW_LOGICAL_WIDTH);
    const float vh = static_cast<float>(WINDOW_LOGICAL_HIGHT);

    if (state->texBackground != nullptr)
    {
        const SDL_FRect src = {state->bgScrollX, state->bgScrollY, vw, vh};

        SDL_RenderTexture(renderer, state->texBackground, &src, nullptr);
    }

    if (state->texArena != nullptr)
    {
        const SDL_FRect src = {state->scrollX, state->scrollY, vw, vh};

        SDL_RenderTexture(renderer, state->texArena, &src, nullptr);
    }

    if (state->texForeground != nullptr)
    {
        const SDL_FRect src = {state->fgScrollX, state->fgScrollY, vw, vh};

        SDL_RenderTexture(renderer, state->texForeground, &src, nullptr);
    }

    SDL_RenderPresent(renderer);
}
