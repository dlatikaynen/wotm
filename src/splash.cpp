#include "inc/splash.h"
#include "inc/platform.h"
#include "inc/synth.h"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cairo.h>
#include <cairo-ft.h>
#include <ft2build.h>
#include <format>
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
        const double cyc  = gamestate.i * 0.06 + phase;
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

        cairo_matrix_init_translate(&cm, -gamestate.i * (cycleW / 80.0), 0.0);
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

    int scancode_to_note(SDL_Scancode sc)
    {
        switch (sc)
        {
            case SDL_SCANCODE_Z: return 60; // C4
            case SDL_SCANCODE_S: return 61; // C#4
            case SDL_SCANCODE_X: return 62; // D4
            case SDL_SCANCODE_D: return 63; // D#4
            case SDL_SCANCODE_C: return 64; // E4
            case SDL_SCANCODE_V: return 65; // F4
            case SDL_SCANCODE_G: return 66; // F#4
            case SDL_SCANCODE_B: return 67; // G4
            case SDL_SCANCODE_H: return 68; // G#4
            case SDL_SCANCODE_N: return 69; // A4 (440 Hz)
            case SDL_SCANCODE_J: return 70; // A#4
            case SDL_SCANCODE_M: return 71; // B4
            case SDL_SCANCODE_COMMA: return 72; // C5

            case SDL_SCANCODE_Q: return 72; // C5
            case SDL_SCANCODE_2: return 73; // C#5
            case SDL_SCANCODE_W: return 74; // D5
            case SDL_SCANCODE_3: return 75; // D#5
            case SDL_SCANCODE_E: return 76; // E5
            case SDL_SCANCODE_R: return 77; // F5
            case SDL_SCANCODE_5: return 78; // F#5
            case SDL_SCANCODE_T: return 79; // G5
            case SDL_SCANCODE_6: return 80; // G#5
            case SDL_SCANCODE_Y: return 81; // A5
            case SDL_SCANCODE_7: return 82; // A#5
            case SDL_SCANCODE_U: return 83; // B5
            case SDL_SCANCODE_I: return 84; // C6

            default:                      
                return -1;
        }
    }

    synth::Waveform splashWaveform = synth::Waveform::Saw;
}

void splash_step(void *userData)
{
    const auto& state = static_cast<Gamestate*>(userData);
    const auto& i = state->i;

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
    state->i += 0.1;


    // temp debugging aid: go straight into a level
    // state->screen = WOTM_SCREEN_LODING;
    //state->enteringLevel = 1;


    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
            {
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                //state.width = event.window.data1;
                //state.height = event.window.data2;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                if (!event.key.repeat)
                {
                    switch(event.key.scancode)
                    {
                        case SDL_SCANCODE_SPACE:
                        {
                            state->screen = WOTM_SCREEN_LODING;
                            state->enteringLevel = 1;

                            break;
                        }

                        case SDL_SCANCODE_A:
                        {
                            const auto& cli = detect_client_info();

                            SDL_ShowSimpleMessageBox(
                                SDL_MESSAGEBOX_ERROR,
                                "Error",
                                std::format("{} has experienced an error and needs to close. Did you drop your {} into a reactor core?", cli.browser, cli.deviceType).c_str(),
                                window
                            );

                            break;
                        }

                        case SDL_SCANCODE_TAB:
                        {
                            const int next = (static_cast<int>(splashWaveform) + 1) % static_cast<int>(synth::Waveform::COUNT);
                            splashWaveform = static_cast<synth::Waveform>(next);
                            synth::set_waveform(splashWaveform);

                            break;
                        }

                        default:
                        {
                            const int note = scancode_to_note(event.key.scancode);
                            if (note >= 0)
                            {
                                synth::note_on(note);
                            }

                            break;
                        }
                    }
                }

                break;
            }
            case SDL_EVENT_KEY_UP:
            {
                const int note = scancode_to_note(event.key.scancode);
                
                if (note >= 0)
                {
                    synth::note_off(note);
                }

                break;
            }
        }
    }
}