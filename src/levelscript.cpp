#include "inc/levelscript.h"

#include <SDL3/SDL.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <sol/sol.hpp>

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>

namespace
{
    using SurfacePtr = std::shared_ptr<cairo_surface_t>;

    // shared by all gfx functions
    struct FontFace
    {
        FT_Face ft = nullptr;
        cairo_font_face_t* cf = nullptr;
    };

    struct Recorder
    {
        std::vector<DrawCmd>& cmds;
        LevelLighting& light;
        FontFace* font = nullptr;
        
        // default upper-left
        double sunRad = 135.0 * M_PI / 180.0;
        
        int commandsPerFrame = 1;
    };

    SurfacePtr load_png_surface(const std::string& path)
    {
        SDL_Surface* raw = SDL_LoadPNG(path.c_str());

        if (raw == nullptr)
        {
            return nullptr;
        }

        SDL_Surface* conv = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ARGB8888);
        SDL_DestroySurface(raw);
        if (conv == nullptr)
        {
            return nullptr;
        }

        const int w = conv->w;
        const int h = conv->h;
        cairo_surface_t* cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        cairo_surface_flush(cs);

        unsigned char* dst = cairo_image_surface_get_data(cs);
        const int dstride = cairo_image_surface_get_stride(cs);

        SDL_LockSurface(conv);

        const auto* srcPixels = static_cast<const unsigned char*>(conv->pixels);
        const int sstride = conv->pitch;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const unsigned char* sp = srcPixels + y * sstride + x * 4;
                const unsigned int b = sp[0], g = sp[1], r = sp[2], a = sp[3];
                unsigned char* dp = dst + y * dstride + x * 4;

                dp[0] = static_cast<unsigned char>(b * a / 255);
                dp[1] = static_cast<unsigned char>(g * a / 255);
                dp[2] = static_cast<unsigned char>(r * a / 255);
                dp[3] = static_cast<unsigned char>(a);
            }
        }

        SDL_UnlockSurface(conv);
        SDL_DestroySurface(conv);
        cairo_surface_mark_dirty(cs);
        
        return SurfacePtr(cs, cairo_surface_destroy);
    }

    // cairo's fface references FT's
    FontFace* get_font(const std::string& path)
    {
        static std::map<std::string, FontFace> cache;
        static FT_Library ft = nullptr;
        auto it = cache.find(path);
        
        if (it != cache.end())
        {
            return &it->second;
        }

        if (ft == nullptr && FT_Init_FreeType(&ft) != 0)
        {
            return nullptr;
        }

        FT_Face face = nullptr;

        if (FT_New_Face(ft, path.c_str(), 0, &face) != 0)
        {
            return nullptr;
        }

        FontFace ff{face, cairo_ft_font_face_create_for_ft_face(face, 0)};
        
        return &cache.emplace(path, ff).first->second;
    }

    cairo_operator_t op_from(const std::string& s)
    {
        if (s == "multiply") 
        {
            return CAIRO_OPERATOR_MULTIPLY;
        }

        if (s == "screen")
        {
            return CAIRO_OPERATOR_SCREEN;
        }

        if (s == "add")
        {
            return CAIRO_OPERATOR_ADD;
        }

        if (s == "overlay")
        {
            return CAIRO_OPERATOR_OVERLAY;
        }

        if (s == "darken") 
        {
            return CAIRO_OPERATOR_DARKEN;
        }

        if (s == "lighten")
        {
            return CAIRO_OPERATOR_LIGHTEN;
        }

        if (s == "source")
        {
            return CAIRO_OPERATOR_SOURCE;
        }

        return CAIRO_OPERATOR_OVER;
    }

    cairo_line_cap_t cap_from(const std::string& s)
    {
        if (s == "round") 
        {
            return CAIRO_LINE_CAP_ROUND;
        }

        if (s == "square") 
        {
            return CAIRO_LINE_CAP_SQUARE;
        }

        return CAIRO_LINE_CAP_BUTT;
    }

    cairo_line_join_t join_from(const std::string& s)
    {
        if (s == "round") 
        {
            return CAIRO_LINE_JOIN_ROUND;
        }

        if (s == "bevel") 
        {
            return CAIRO_LINE_JOIN_BEVEL;
        }

        return CAIRO_LINE_JOIN_MITER;
    }

    double clamp01(double v)
    {
        return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); 
    }

    void register_gfx(sol::state& lua, Recorder& rec)
    {
        sol::table g = lua.create_named_table("gfx");

        // sources, states
        g.set_function("rgb", [&rec](double r, double gg, double b) {
            rec.cmds.push_back([r, gg, b](cairo_t* cr) { cairo_set_source_rgb(cr, r, gg, b); });
        });

        g.set_function("rgba", [&rec](double r, double gg, double b, double a) {
            rec.cmds.push_back([r, gg, b, a](cairo_t* cr) { cairo_set_source_rgba(cr, r, gg, b, a); });
        });

        g.set_function("line_width", [&rec](double w) {
            rec.cmds.push_back([w](cairo_t* cr) { cairo_set_line_width(cr, w); });
        });

        g.set_function("line_cap", [&rec](std::string s) {
            const auto c = cap_from(s);
            rec.cmds.push_back([c](cairo_t* cr) { cairo_set_line_cap(cr, c); });
        });

        g.set_function("line_join", [&rec](std::string s) {
            const auto j = join_from(s);
            rec.cmds.push_back([j](cairo_t* cr) { cairo_set_line_join(cr, j); });
        });

        g.set_function("operator", [&rec](std::string s) {
            const auto o = op_from(s);
            rec.cmds.push_back([o](cairo_t* cr) { cairo_set_operator(cr, o); });
        });

        // transforms
        g.set_function("save",      [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_save(cr); }); });
        g.set_function("restore",   [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_restore(cr); }); });
        g.set_function("translate", [&rec](double x, double y) { rec.cmds.push_back([x, y](cairo_t* cr) { cairo_translate(cr, x, y); }); });
        g.set_function("scale",     [&rec](double x, double y) { rec.cmds.push_back([x, y](cairo_t* cr) { cairo_scale(cr, x, y); }); });
        g.set_function("rotate",    [&rec](double a) { rec.cmds.push_back([a](cairo_t* cr) { cairo_rotate(cr, a); }); });
        g.set_function("identity",  [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_identity_matrix(cr); }); });

        // paths
        g.set_function("move_to",     [&rec](double x, double y) { rec.cmds.push_back([x, y](cairo_t* cr) { cairo_move_to(cr, x, y); }); });
        g.set_function("line_to",     [&rec](double x, double y) { rec.cmds.push_back([x, y](cairo_t* cr) { cairo_line_to(cr, x, y); }); });
        g.set_function("rel_line_to", [&rec](double x, double y) { rec.cmds.push_back([x, y](cairo_t* cr) { cairo_rel_line_to(cr, x, y); }); });
        g.set_function("curve_to",    [&rec](double x1, double y1, double x2, double y2, double x3, double y3) {
            rec.cmds.push_back([x1, y1, x2, y2, x3, y3](cairo_t* cr) 
            {
                 cairo_curve_to(cr, x1, y1, x2, y2, x3, y3); 
            });
        });

        g.set_function("arc", [&rec](double xc, double yc, double r, double a1, double a2) {
            rec.cmds.push_back([xc, yc, r, a1, a2](cairo_t* cr) { cairo_arc(cr, xc, yc, r, a1, a2); });
        });

        g.set_function("arc_neg", [&rec](double xc, double yc, double r, double a1, double a2) {
            rec.cmds.push_back([xc, yc, r, a1, a2](cairo_t* cr) { cairo_arc_negative(cr, xc, yc, r, a1, a2); });
        });

        g.set_function("rectangle", [&rec](double x, double y, double w, double h) {
            rec.cmds.push_back([x, y, w, h](cairo_t* cr) { cairo_rectangle(cr, x, y, w, h); });
        });

        g.set_function("circle", [&rec](double xc, double yc, double r) {
            rec.cmds.push_back([xc, yc, r](cairo_t* cr) {
                cairo_new_sub_path(cr);
                cairo_arc(cr, xc, yc, r, 0.0, 2.0 * M_PI);
            });
        });

        g.set_function("rounded_rect", [&rec](double x, double y, double w, double h, double r) {
            rec.cmds.push_back([x, y, w, h, r](cairo_t* cr) {
                const double d = M_PI / 180.0;
                cairo_new_sub_path(cr);
                cairo_arc(cr, x + w - r, y + r,     r, -90 * d,   0 * d);
                cairo_arc(cr, x + w - r, y + h - r, r,   0 * d,  90 * d);
                cairo_arc(cr, x + r,     y + h - r, r,  90 * d, 180 * d);
                cairo_arc(cr, x + r,     y + r,     r, 180 * d, 270 * d);
                cairo_close_path(cr);
            });
        });

        g.set_function("close",        [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_close_path(cr); }); });
        g.set_function("new_path",     [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_new_path(cr); }); });
        g.set_function("new_sub_path", [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_new_sub_path(cr); }); });

        // painting
        g.set_function("fill",            [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_fill(cr); }); });
        g.set_function("fill_preserve",   [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_fill_preserve(cr); }); });
        g.set_function("stroke",          [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_stroke(cr); }); });
        g.set_function("stroke_preserve", [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_stroke_preserve(cr); }); });
        g.set_function("paint",           [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_paint(cr); }); });
        g.set_function("paint_alpha",     [&rec](double a) { rec.cmds.push_back([a](cairo_t* cr) { cairo_paint_with_alpha(cr, a); }); });
        g.set_function("clip",            [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_clip(cr); }); });
        g.set_function("clip_preserve",   [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_clip_preserve(cr); }); });
        g.set_function("reset_clip",      [&rec]() { rec.cmds.push_back([](cairo_t* cr) { cairo_reset_clip(cr); }); });

        // gradients
        auto read_stops = [](sol::table stops) {
            std::vector<std::array<double, 5>> v;
            const std::size_t n = stops.size();
            
            for (std::size_t i = 1; i <= n; ++i)
            {
                sol::table s = stops[i];
                sol::optional<double> a = s[5];

                v.push_back({
                    s[1].get<double>(),
                    s[2].get<double>(),
                    s[3].get<double>(),
                    s[4].get<double>(),
                    a.value_or(1.0)
                });
            }

            return v;
        };

        g.set_function("linear", [&rec, read_stops](double x0, double y0, double x1, double y1, sol::table stops) {
            auto v = read_stops(stops);
            rec.cmds.push_back([=](cairo_t* cr) {
                cairo_pattern_t* p = cairo_pattern_create_linear(x0, y0, x1, y1);

                for (const auto& s : v)
                {
                     cairo_pattern_add_color_stop_rgba(p, s[0], s[1], s[2], s[3], s[4]);
                }

                cairo_set_source(cr, p);
                cairo_pattern_destroy(p);
            });
        });

        g.set_function("radial", [&rec, read_stops](double cx0, double cy0, double r0, double cx1, double cy1, double r1, sol::table stops) {
            auto v = read_stops(stops);
            rec.cmds.push_back([=](cairo_t* cr) {
                cairo_pattern_t* p = cairo_pattern_create_radial(cx0, cy0, r0, cx1, cy1, r1);
                
                for (const auto& s : v) 
                {
                    cairo_pattern_add_color_stop_rgba(p, s[0], s[1], s[2], s[3], s[4]);
                }

                cairo_set_source(cr, p);
                cairo_pattern_destroy(p);
            });
        });

        // draw a png with transparency. alpha = 1, scale = 1
        g.set_function("image", [&rec](std::string path, double x, double y, sol::optional<sol::table> opts) {
            SurfacePtr surf = load_png_surface(path);
            if (surf == nullptr)
            {
                std::cerr << "[gfx] failed to load the \"" << path << "\" image\n";
            
                return;
            }
            
            double alpha = 1.0, scale = 1.0;
            
            if (opts)
            {
                alpha = opts->get_or("alpha", 1.0);
                scale = opts->get_or("scale", 1.0);
            }

            rec.cmds.push_back([surf, x, y, alpha, scale](cairo_t* cr) {
                cairo_save(cr);
                cairo_translate(cr, x, y);
                if (scale != 1.0) 
                {
                    cairo_scale(cr, scale, scale);
                }

                cairo_set_source_surface(cr, surf.get(), 0, 0);
                if (alpha >= 1.0) {cairo_paint(cr);
                }
                else
                {
                    cairo_paint_with_alpha(cr, alpha);
                }

                cairo_restore(cr);
            });
        });

        // set the current source to a (optionally repeating) PNG pattern; the
        // script then fills a path with it
        g.set_function("image_pattern", [&rec](std::string path, sol::optional<bool> repeat) {
            SurfacePtr surf = load_png_surface(path);

            if (surf == nullptr)
            {
                std::cout << "[gfx] image_pattern: failed to load " << path << "\n";

                return;
            }

            const bool rep = repeat.value_or(false);

            rec.cmds.push_back([surf, rep](cairo_t* cr) {
                cairo_pattern_t* p = cairo_pattern_create_for_surface(surf.get());

                cairo_pattern_set_extend(p, rep ? CAIRO_EXTEND_REPEAT : CAIRO_EXTEND_PAD);
                cairo_set_source(cr, p);
                cairo_pattern_destroy(p);
            });
        });

        // select the current font for text/glyph/text_path
        g.set_function("font", [&rec](std::string path) {
            FontFace* f = get_font(path);

            if (f == nullptr)
            {
                 std::cerr << "[gfx] failed to load the \"" << path << "\" font\n";
            }

            rec.font = f;
        });

        auto emit_text = [&rec](const std::string& str, double x, double y, double size, bool fill) {
            FontFace* f = rec.font;
            rec.cmds.push_back([f, str, x, y, size, fill](cairo_t* cr) {
                if (f != nullptr)
                {
                     cairo_set_font_face(cr, f->cf);
                }

                cairo_set_font_size(cr, size);
                cairo_move_to(cr, x, y);
                cairo_text_path(cr, str.c_str());
                if (fill)
                {
                    cairo_fill(cr);
                }
            });
        };

        g.set_function("text", [emit_text](std::string str, double x, double y, double size) {
            emit_text(str, x, y, size, true);
        });

        g.set_function("text_path", [emit_text](std::string str, double x, double y, double size) {
            emit_text(str, x, y, size, false);
        });

        g.set_function("glyph", [&rec](unsigned int codepoint, double x, double y, double size) {
            FontFace* f = rec.font;
            rec.cmds.push_back([f, codepoint, x, y, size](cairo_t* cr) {
                if (f == nullptr)
                {
                    return;
                }

                cairo_set_font_face(cr, f->cf);
                cairo_set_font_size(cr, size);
                
                const FT_UInt idx = FT_Get_Char_Index(f->ft, codepoint);                
                cairo_glyph_t gl{idx, x, y};

                cairo_glyph_path(cr, &gl, 1);
                cairo_fill(cr);
            });
        });

        // sun direction in deg for lit_fill
        g.set_function("sun", [&rec](double deg) { rec.sunRad = deg * M_PI / 180.0; });

        // recorded commands per frame >= 1
        g.set_function("commands_per_frame", [&rec](int n) { rec.commandsPerFrame = n > 0 ? n : 1; });

        // fill the current path with base color (r,g,b) and bevel its edges with
        // highlight and shadow
        // depth = bevel width in px, intensity 0..1
        g.set_function("lit_fill", [&rec](double r, double gg, double b, double depth, double intensity) {
            const double sun = rec.sunRad;

            rec.cmds.push_back([r, gg, b, depth, intensity, sun](cairo_t* cr) {
                cairo_path_t* path = cairo_copy_path(cr);
                
                // fill
                cairo_new_path(cr);
                cairo_append_path(cr, path);
                cairo_set_source_rgb(cr, r, gg, b);
                cairo_fill(cr);
                // bevel
                cairo_save(cr);
                cairo_new_path(cr);
                cairo_append_path(cr, path);
                cairo_clip(cr);

                // highlight goes in-direction, shadow on the opposite edge.
                const double dx = std::cos(sun) * depth;
                const double dy = -std::sin(sun) * depth;

                cairo_set_line_width(cr, depth * 2.0);
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
                // highlight on the sun-facing edge
                cairo_save(cr);
                cairo_translate(cr, -dx, -dy);
                cairo_new_path(cr);
                cairo_append_path(cr, path);
                cairo_set_source_rgba(
                    cr,
                    clamp01(r + intensity),
                    clamp01(gg + intensity),
                    clamp01(b + intensity),
                    0.85
                );

                cairo_stroke(cr);
                cairo_restore(cr);
                // shadow on the opposite edge
                cairo_save(cr);
                cairo_translate(cr, dx, dy);
                cairo_new_path(cr);
                cairo_append_path(cr, path);
                cairo_set_source_rgba(
                    cr,
                    r * (1.0 - intensity),
                    gg * (1.0 - intensity),
                    b * (1.0 - intensity),
                    0.85
                );

                cairo_stroke(cr);
                cairo_restore(cr);
                // drop clip
                cairo_restore(cr);
                cairo_new_path(cr);
                cairo_path_destroy(path);
            });
        });

        // scene lighting
        g.set_function("ambient", [&rec](double r, double gg, double b) {
            rec.light.hasAmbient = true;
            rec.light.ar = r; rec.light.ag = gg; rec.light.ab = b;
        });

        g.set_function("light", [&rec](double x, double y, double radius, double r, double gg, double b, double intensity) {
            rec.light.lights.push_back({x, y, radius, r, gg, b, intensity});
        });
    }
}

bool run_level_script(
    const std::string& path,
    std::vector<DrawCmd>& out,
    LevelLighting& light,
    int& commandsPerFrame,
    std::string& err
) {
    out.clear();
    light = LevelLighting{};

    try
    {
        sol::state lua;

        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);

        Recorder rec{out, light};

        register_gfx(lua, rec);

        auto result = lua.safe_script_file(path, sol::script_pass_on_error);

        if (!result.valid())
        {
            sol::error e = result;

            err = e.what();
            out.clear();
            light = LevelLighting{};

            return false;
        }

        commandsPerFrame = rec.commandsPerFrame;
    }
    catch (const std::exception& e)
    {
        err = e.what();
        out.clear();
        light = LevelLighting{};

        return false;
    }

    return true;
}
