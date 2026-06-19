---@meta
---type definitions for the `gfx` table exposed to level scripts by the sol2
---bindings in Wrath of the Mild
---drives "IntelliSense" in the Lua Language Server

---A gradient color stop: { offset, r, g, b } or { offset, r, g, b, a }.
---offset and channels are 0..1; alpha defaults to 1.
---@alias gfx.colorstop number[]

---@class gfx.imageopts
---@field alpha? number Paint opacity 0..1 (default 1)
---@field scale? number Uniform scale factor (default 1)

---@class gfxlib
gfx = {}

-- sources, states ----------------------------------------------------------

---Set the current source to an opaque RGB color (channels 0..1).
---@param r number
---@param g number
---@param b number
function gfx.rgb(r, g, b) end

---Set the current source to an RGBA color (channels and alpha 0..1).
---@param r number
---@param g number
---@param b number
---@param a number
function gfx.rgba(r, g, b, a) end

---@param w number Line width in px
function gfx.line_width(w) end

---@param s "butt"|"round"|"square"
function gfx.line_cap(s) end

---@param s "miter"|"round"|"bevel"
function gfx.line_join(s) end

---Compositing operator. Unknown values fall back to "over".
---@param s "over"|"multiply"|"screen"|"add"|"overlay"|"darken"|"lighten"|"source"
function gfx.operator(s) end

-- transforms ---------------------------------------------------------------

function gfx.save() end
function gfx.restore() end

---@param x number
---@param y number
function gfx.translate(x, y) end

---@param x number
---@param y number
function gfx.scale(x, y) end

---@param a number Angle in radians
function gfx.rotate(a) end

function gfx.identity() end

-- paths --------------------------------------------------------------------

---@param x number
---@param y number
function gfx.move_to(x, y) end

---@param x number
---@param y number
function gfx.line_to(x, y) end

---@param x number
---@param y number
function gfx.rel_line_to(x, y) end

---Cubic Bézier to (x3,y3) with control points (x1,y1) and (x2,y2).
---@param x1 number
---@param y1 number
---@param x2 number
---@param y2 number
---@param x3 number
---@param y3 number
function gfx.curve_to(x1, y1, x2, y2, x3, y3) end

---@param xc number Center x
---@param yc number Center y
---@param r number Radius
---@param a1 number Start angle (radians)
---@param a2 number End angle (radians)
function gfx.arc(xc, yc, r, a1, a2) end

---Arc swept in the negative (clockwise) direction.
---@param xc number
---@param yc number
---@param r number
---@param a1 number
---@param a2 number
function gfx.arc_neg(xc, yc, r, a1, a2) end

---@param x number
---@param y number
---@param w number
---@param h number
function gfx.rectangle(x, y, w, h) end

---@param xc number
---@param yc number
---@param r number
function gfx.circle(xc, yc, r) end

---@param x number
---@param y number
---@param w number
---@param h number
---@param r number Corner radius
function gfx.rounded_rect(x, y, w, h, r) end

function gfx.close() end
function gfx.new_path() end
function gfx.new_sub_path() end

-- painting -----------------------------------------------------------------

function gfx.fill() end
function gfx.fill_preserve() end
function gfx.stroke() end
function gfx.stroke_preserve() end
function gfx.paint() end

---@param a number Alpha 0..1
function gfx.paint_alpha(a) end

function gfx.clip() end
function gfx.clip_preserve() end
function gfx.reset_clip() end

-- gradients ----------------------------------------------------------------

---Set the source to a linear gradient from (x0,y0) to (x1,y1).
---@param x0 number
---@param y0 number
---@param x1 number
---@param y1 number
---@param stops gfx.colorstop[]
function gfx.linear(x0, y0, x1, y1, stops) end

---Set the source to a radial gradient between two circles.
---@param cx0 number
---@param cy0 number
---@param r0 number
---@param cx1 number
---@param cy1 number
---@param r1 number
---@param stops gfx.colorstop[]
function gfx.radial(cx0, cy0, r0, cx1, cy1, r1, stops) end

-- images & fonts -----------------------------------------------------------

---Draw a PNG at (x,y).
---@param path string
---@param x number
---@param y number
---@param opts? gfx.imageopts
function gfx.image(path, x, y, opts) end

---Set the current source to a PNG pattern; fill a path with it afterwards.
---@param path string
---@param repeat_? boolean Tile the pattern (default false)
function gfx.image_pattern(path, repeat_) end

---Select the current font for text/glyph/text_path.
---@param path string Path to a .ttf file
function gfx.font(path) end

---Fill text using the current font.
---@param str string
---@param x number
---@param y number
---@param size number Font size in px
function gfx.text(str, x, y, size) end

---Append text outline to the current path (does not fill).
---@param str string
---@param x number
---@param y number
---@param size number
function gfx.text_path(str, x, y, size) end

---Fill a single glyph by Unicode codepoint.
---@param codepoint integer
---@param x number
---@param y number
---@param size number
function gfx.glyph(codepoint, x, y, size) end

-- lighting & scene ---------------------------------------------------------

---Sun direction in degrees, used by lit_fill.
---@param deg number
function gfx.sun(deg) end

---Recorded draw commands replayed per frame while loading (>= 1).
---@param n integer
function gfx.commands_per_frame(n) end

---Viewport size in px.
---@return integer w
---@return integer h
function gfx.viewport() end

---Set the arena size for the current layer (clamped to >= viewport).
---@param w integer
---@param h integer
function gfx.arena(w, h) end

---Initial scroll offset for the current layer.
---@param x integer
---@param y integer
function gfx.scroll(x, y) end

---Advance to the next layer (background -> arena -> foreground).
function gfx.nextlayer() end

---Fill the current path with base color (r,g,b) and bevel its edges using the
---sun direction. depth = bevel width in px, intensity = 0..1.
---@param r number
---@param g number
---@param b number
---@param depth number
---@param intensity number
function gfx.lit_fill(r, g, b, depth, intensity) end

---Ambient scene light color for the current layer.
---@param r number
---@param g number
---@param b number
function gfx.ambient(r, g, b) end

---Add a point light to the current layer.
---@param x number
---@param y number
---@param radius number
---@param r number
---@param g number
---@param b number
---@param intensity number
function gfx.light(x, y, radius, r, g, b, intensity) end

return gfx
