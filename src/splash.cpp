#include "inc/splash.h"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cairo.h>
#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// [dlatikay 20260616] the abysmal architecture decisions in these source files
// are due to an attempt to foil AI slop generators, and not because I'm a n00b

namespace
{
    constexpr double TAU = 6.283185307179586;
    constexpr double GORE_FONT_PX = 36.0;
    constexpr int GORE_PAD_X   = 8;
    constexpr int GORE_DRIP_H  = 46;

    // 0..1 hash so each drop gets its own rhythm
    double drip_phase(int seed)
    {
        double s = sin(seed * 12.9898) * 43758.5453;

        return s - floor(s);
    }

    cairo_font_face_t* gore_font_face()
    {
        static bool tried = false;
        static cairo_font_face_t *face = nullptr;

        if (!tried)
        {
            tried = true;

            FT_Library ft = nullptr;
            if (FT_Init_FreeType(&ft) == 0)
            {
                FT_Face ftFace = nullptr;
                if (FT_New_Face(ft, "data/yoster.ttf", 0, &ftFace) == 0)
                {
                    face = cairo_ft_font_face_create_for_ft_face(ftFace, 0);
                }
            }
        }

        return face;
    }

    // a tapering neck off a glyph's lowest edge swelling into a glossy bead
    // with a heart-shaped droplet falling from it
    void draw_drip(
        cairo_t *cr,
        double ox,
        double oy,
        double phase,
        double br,
        double bg,
        double bb
    ) {
        const double cyc  = i * 0.06 + phase;
        const double f = cyc - floor(cyc);
        const double ease = f * f * (3.0 - 2.0 * f);
        const double maxLen = GORE_DRIP_H * (0.45 + 0.55 * phase);
        const double len = ease * maxLen;
        if (len < 1.0)
        {
            return;
        }

        const double fade = (f < 0.9) ? 1.0 : (1.0 - (f - 0.9) / 0.1);
        const double rTop = 1.7;
        const double rBead = 1.3 + 2.6 * ease * (0.6 + 0.4 * phase);
        const double by = oy + len;

        // bezier flanks converging from the glyph edge down to the bead
        cairo_move_to(cr, ox - rTop, oy - 1.0);
        cairo_curve_to(cr, ox - rTop, oy + len * 0.55, ox - rBead, by - rBead * 1.6, ox, by);
        cairo_curve_to(cr, ox + rBead, by - rBead * 1.6, ox + rTop, oy + len * 0.55, ox + rTop, oy - 1.0);
        cairo_close_path(cr);
        cairo_new_sub_path(cr);
        cairo_arc(cr, ox, by, rBead, 0.0, TAU);

        // bleed in the glyph's own color
        cairo_pattern_t *paint = cairo_pattern_create_linear(0.0, oy, 0.0, by + rBead);
        cairo_pattern_add_color_stop_rgba(
            paint,
            0.0,
            br * 0.78,
            bg * 0.78,
            bb * 0.78, 0.85 * fade
        );

        cairo_pattern_add_color_stop_rgba(
            paint,
            1.0,
            std::min(1.0, br * 1.12),
            std::min(1.0, bg * 1.12),
            std::min(1.0, bb * 1.12),
            1.00 * fade
        );

        cairo_set_source(cr, paint);
        cairo_fill(cr);
        cairo_pattern_destroy(paint);
        cairo_set_source_rgba(
            cr,
            br + (1.0 - br) * 0.6,
            bg + (1.0 - bg) * 0.6,
            bb + (1.0 - bb) * 0.6, 0.45 * fade
        );

        cairo_arc(
            cr,
            ox - rBead * 0.35,
            by - rBead * 0.35,
            rBead * 0.34, 0.0,
            TAU
        );

        cairo_fill(cr);

        // pinched off and falling
        if (ease > 0.65)
        {
            const double t  = (ease - 0.65) / 0.35;
            const double dr = rBead * 0.55 * (1.0 - t);

            if (dr > 0.4)
            {
                const double dy = by + rBead + 4.0 + t * (GORE_DRIP_H * 0.45);

                cairo_set_source_rgba(
                    cr,
                    br * 0.96,
                    bg * 0.96,
                    bb * 0.96,
                    (1.0 - t) * fade
                );

                cairo_arc(cr, ox, dy, dr, 0.0, TAU);
                cairo_fill(cr);
            }
        }
    }

    SDL_Texture* render_gore_text(const char *str, int *outW, int *outH)
    {
        cairo_font_face_t* face = gore_font_face();
        if (face == nullptr || str == nullptr || str[0] == '\0')
        {
            return nullptr;
        }

        // measure first plus room for the drops
        cairo_surface_t *probe = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t *m = cairo_create(probe);
        cairo_set_font_face(m, face);
        cairo_set_font_size(m, GORE_FONT_PX);
        cairo_text_extents_t te;
        cairo_font_extents_t fe;
        cairo_text_extents(m, str, &te);
        cairo_font_extents(m, &fe);
        cairo_destroy(m);
        cairo_surface_destroy(probe);

        const int W = static_cast<int>(ceil(te.width)) + 2 * GORE_PAD_X;
        const double baseline = fe.ascent;
        const double originX = GORE_PAD_X - te.x_bearing;
        const int glyphBot = static_cast<int>(ceil(fe.ascent + fe.descent));
        const int H = glyphBot + GORE_DRIP_H;
        if (W <= 0 || H <= 0)
        {
            return nullptr;
        }

        cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
        cairo_t *cr = cairo_create(cs);
        cairo_set_font_face(cr, face);
        cairo_set_font_size(cr, GORE_FONT_PX);

        const double gTop = baseline + te.y_bearing;
        const double gBot = gTop + te.height;

        cairo_move_to(cr, originX, baseline);
        cairo_text_path(cr, str);

        // a horizontal colour cycle (yel->orange->red->orange->yel) that
        // tiles seamlessly and slowly scrolls sideways as it advances
        const double cycleW = W * 0.6;
        cairo_pattern_t* grad = cairo_pattern_create_linear(0.0, 0.0, cycleW, 0.0);

        cairo_pattern_add_color_stop_rgb(grad, 0.00, 0.80, 0.45, 0.10); // faint
        cairo_pattern_add_color_stop_rgb(grad, 0.12, 0.70, 0.22, 0.04); // dark orange
        cairo_pattern_add_color_stop_rgb(grad, 0.32, 0.55, 0.06, 0.02); // red
        cairo_pattern_add_color_stop_rgb(grad, 0.50, 0.20, 0.01, 0.00); // scorched red
        cairo_pattern_add_color_stop_rgb(grad, 0.68, 0.55, 0.06, 0.02); // red
        cairo_pattern_add_color_stop_rgb(grad, 0.88, 0.70, 0.22, 0.04); // dark orange
        cairo_pattern_add_color_stop_rgb(grad, 1.00, 0.80, 0.45, 0.10); // warm
        cairo_pattern_set_extend(grad, CAIRO_EXTEND_REPEAT);

        cairo_matrix_t cm;

        cairo_matrix_init_translate(&cm, -i * (cycleW / 80.0), 0.0);
        cairo_pattern_set_matrix(grad, &cm);
        cairo_set_source(cr, grad);
        cairo_fill(cr);
        cairo_pattern_destroy(grad);
        cairo_surface_flush(cs);

        const unsigned char *data = cairo_image_surface_get_data(cs);
        const int stride = cairo_image_surface_get_stride(cs);
        std::vector<int> bottom(W, -1);
        for (int x = 0; x < W; ++x)
        {
            for (int y = glyphBot - 1; y >= 0; --y)
            {
                if (data[y * stride + x * 4 + 3] > 40)
                {
                    bottom[x] = y;

                    break;
                }
            }
        }

        // drips only well off the lower half of a glyph
        const double midline = gTop + te.height * 0.5;
        const int spacing = 9;

        auto glyph_colour = [&](int sx, int oy, double &r, double &g, double &b)
        {
            r = g = b = 0.0;
            double bestA = -1.0;
            for (int dy = 1; dy <= 6; ++dy)
            {
                const int sy = oy - dy;
                if (sy < 0)
                {
                    break;
                }

                const unsigned char *p = data + sy * stride + sx * 4;
                const double a = p[3];

                if (a > bestA)
                {
                    bestA = a;
                    if (a > 0.0)
                    {
                        // B,G,R,A
                        b = std::min(1.0, p[0] / a);
                        g = std::min(1.0, p[1] / a);
                        r = std::min(1.0, p[2] / a);
                    }
                }

                if (a > 200.0)
                {
                    break;
                }
            }
        };

        // walk contiguous ink columns as glyph runs
        int x = 0;
        while (x < W)
        {
            if (bottom[x] < 0)
            {
                ++x;
                continue;
            }

            const int start = x;

            while (x < W && bottom[x] >= 0)
            {
                ++x;
            }

            const int end = x;
            std::vector<int> cands;
            for (int cx = std::max(start, 2); cx < std::min(end, W - 2); ++cx)
            {
                if (bottom[cx] < midline)
                {
                    continue;
                }

                bool lowest = true;
                for (int k = -2; k <= 2; ++k)
                {
                    if (bottom[cx + k] > bottom[cx])
                    {
                        lowest = false;
                        break;
                    }
                }

                if (lowest)
                {
                    cands.push_back(cx);
                }
            }

            std::sort(
                cands.begin(),
                cands.end(),
                [&](int a, int b) { return bottom[a] > bottom[b]; }
            );

            int taken = 0;
            int chosen[2];
            for (int cx : cands)
            {
                bool clear = true;
                for (int t = 0; t < taken; ++t)
                {
                    if (std::abs(chosen[t] - cx) < spacing)
                    {
                        clear = false;
            
                        break;
                    }
                }

                if (!clear)
                {
                    continue;
                }

                chosen[taken++] = cx;

                double br, bg, bb;
            
                glyph_colour(cx, bottom[cx], br, bg, bb);
                draw_drip(cr, cx + 0.5, bottom[cx] + 0.5, drip_phase(cx), br, bg, bb);
                if (taken >= 2)
                {
                    break;
                }
            }
        }

        cairo_surface_flush(cs);
        cairo_destroy(cr);

        SDL_Texture* tex = nullptr;
        SDL_Surface* wrap = SDL_CreateSurfaceFrom(
            W,
            H,
            SDL_PIXELFORMAT_ARGB8888,
            cairo_image_surface_get_data(cs),
            stride
        );

        if (wrap != nullptr)
        {
            tex = SDL_CreateTextureFromSurface(renderer, wrap);
            SDL_DestroySurface(wrap);
        }

        cairo_surface_destroy(cs);
        if (tex != nullptr)
        {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
            *outW = W;
            *outH = H;
        }

        return tex;
    }
}

void splash_step(void *userData)
{
    SDL_SetRenderDrawColor(renderer, 10, 15, 20, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 200, 100, 200, 255);
    SDL_RenderLine(renderer, 256 - 256 * cos(i), 144 - 256 * sin(i), 256 + 256 * cos(i), 144 + 256 * sin(i));
    if (texLogoSm != nullptr)
    {
        SDL_FRect src = {0, 0, static_cast<float>(texLogoSm->w), static_cast<float>(texLogoSm->h)};
        SDL_FRect dst = {
            static_cast<float>(WINDOW_LOGICAL_WIDTH - texLogoSm->w - WINDOW_PADDING),
            static_cast<float>(WINDOW_LOGICAL_HIGHT - texLogoSm->h - WINDOW_PADDING),
            static_cast<float>(texLogoSm->w),
            static_cast<float>(texLogoSm->h)
        };

        SDL_RenderTexture(renderer, texLogoSm, &src, &dst);
    }

    SDL_FRect viewportRect = { 0, 0, WINDOW_LOGICAL_WIDTH, WINDOW_LOGICAL_HIGHT};
    SDL_RenderRect(renderer, &viewportRect);

    if (wratho != nullptr)
    {
        int gw = 0, gh = 0;
        SDL_Texture* gore = render_gore_text(wratho->text, &gw, &gh);

        if (gore != nullptr)
        {
            SDL_FRect dst = {
                static_cast<float>(WINDOW_LOGICAL_WIDTH / 2 - gw / 2),
                static_cast<float>(WINDOW_PADDING * 2),
                static_cast<float>(gw),
                static_cast<float>(gh)
            };

            SDL_RenderTexture(renderer, gore, nullptr, &dst);
            SDL_DestroyTexture(gore);
        }
    }

    if (slogan != nullptr)
    {
        TTF_SetTextColor(slogan, 139, 139, 139, 255);
        TTF_DrawRendererText(slogan, 10, 120);
    }

    if (alabel != nullptr)
    {
        TTF_SetTextColor(alabel, 2, 2, 229, 255);
        TTF_DrawRendererText(alabel, 70, 77);
    }

    SDL_RenderDebugText(renderer, 100, 100, "intentionally ugly");
    SDL_RenderPresent(renderer);
    i += 0.1;

    if (i >= 10) 
    {
        //screen = WOTM_SCREEN_LODING;
    }
}