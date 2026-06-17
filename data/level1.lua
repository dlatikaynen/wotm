-- TODO: the lua script should be able to get those from the host application
local W, H = 512, 288

-- replay 2 draw commands per frame while loading
gfx.commands_per_frame(2)

-- sky: vertical gradient
gfx.linear(0, 0, 0, H, {
  { 0.0, 0.04, 0.05, 0.12 },
  { 1.0, 0.18, 0.13, 0.24 },
})
gfx.paint()

-- stars
for i = 1, 90 do
  local x = (i * 97) % W
  local y = (i * 53) % 150
  gfx.rgba(1.0, 0.97, 0.85, 0.35 + (i % 5) * 0.12)
  gfx.circle(x, y, 0.5 + (i % 3) * 0.4)
  gfx.fill()
end

-- moon
gfx.image("data/logo-ki-064.png", 372, 24, { alpha = 0.9, scale = 0.55 })

-- ground
gfx.sun(135)
gfx.move_to(0, H)
for x = 0, W, 6 do
  local y = 200 + math.sin(x * 0.025) * 14 + math.sin(x * 0.011 + 1.3) * 22
  gfx.line_to(x, y)
end
gfx.line_to(W, H)
gfx.close()
gfx.lit_fill(0.12, 0.32, 0.18, 3, 0.5)

-- raised platform lit from above
gfx.sun(100)
gfx.rounded_rect(150, 158, 130, 26, 8)
gfx.lit_fill(0.42, 0.36, 0.30, 3, 0.6)

-- panel tiled with img
gfx.image_pattern("data/logo-ki-032.png", true)
gfx.rectangle(24, 206, 96, 60)
gfx.fill()

-- title text
gfx.font("data/yoster.ttf")
gfx.rgb(0.95, 0.85, 0.30)
gfx.text("Hydrogen", 150, 60, 30)

-- edge-lit glyph
gfx.font("data/yoster.ttf")
gfx.sun(90)
gfx.text_path("H", 360, 250, 70)
gfx.lit_fill(0.62, 0.12, 0.12, 4, 0.7)

-- exercises the transform stack
gfx.save()
gfx.translate(64, 70)
gfx.rotate(0.4)
gfx.scale(1.2, 1.2)
gfx.operator("screen")
gfx.line_width(2.0)
gfx.line_cap("round")
gfx.line_join("bevel")
gfx.rgba(0.6, 0.8, 1.0, 0.8)
gfx.new_path()
gfx.move_to(-20, 0)
gfx.rel_line_to(20, -16)
gfx.curve_to(30, 4, -30, 4, -20, 0)
gfx.close()
gfx.stroke()
gfx.operator("over")
gfx.identity()
gfx.restore()

-- halo
gfx.save()
gfx.new_sub_path()
gfx.arc(400, 55, 40, 0, 2 * math.pi)
gfx.clip_preserve()
gfx.radial(400, 55, 4, 400, 55, 40, {
  { 0.0, 1.0, 0.95, 0.80, 0.9 },
  { 1.0, 1.0, 0.95, 0.80, 0.0 },
})
gfx.fill_preserve()
gfx.paint_alpha(0.5)
gfx.clip()
gfx.reset_clip()
gfx.restore()

-- arcs
gfx.rgba(0.9, 0.9, 1.0, 0.5)
gfx.line_width(1.5)
gfx.line_cap("square")
gfx.line_join("round")
gfx.arc(256, 40, 30, 0.2, 1.4)
gfx.stroke()
gfx.arc_neg(256, 40, 24, 1.4, 0.2)
gfx.stroke_preserve()
gfx.new_path()

-- glyph by codepoint
gfx.font("data/yoster.ttf")
gfx.rgb(0.8, 0.9, 1.0)
gfx.glyph(string.byte("O"), 470, 280, 24)

-- lighting
gfx.ambient(0.90, 0.90, 0.95)
gfx.light(400, 55, 150, 1.0, 0.95, 0.80, 1.0) -- moon
gfx.light(215, 158, 90, 1.0, 0.70, 0.30, 0.9) -- light on the platform
