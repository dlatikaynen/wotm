local W, H = gfx.viewport()

-- gfx.arena(W * 2, H) -- arena twice as wide as viewport
-- initial scroll offset:
-- gfx.scroll(0, 0)

gfx.commands_per_frame(2)

--(1) background

-- sky: vertical gradient
gfx.linear(0, 0, 0, H / 2, {
  { 0.0, 0.18, 0.13, 0.24 },
  { 1.0, 0.04, 0.05, 0.12 }
})

gfx.rectangle(0, 0, W, H / 2)
gfx.fill()

gfx.linear(0, H / 2, 0, H, {
  { 0.0, 0.04, 0.05, 0.12 },
  { 1.0, 0.18, 0.13, 0.24 }
})

gfx.rectangle(0, H/2, W, H)
gfx.fill()

--(2) foreground: the v'ger representation of our first element
gfx.nextlayer()

local TAU = 2 * math.pi
local d = 0.31 * W       -- diameter
local ro = d / 2         -- outer radius
local wall = 31          -- wall+bar thickness
local ri = ro - wall     -- inner radius
local cy = H / 2
local mgn = 0.02 * W     -- distance from screen horizontally
local cx1 = mgn + ro     -- l circle center x
local cx2 = W - mgn - ro -- r circle center

-- how far up from where the bar joins the circle is the outline
local a = math.asin((wall / 2) / ro)

gfx.new_path()

-- circles
gfx.new_sub_path()
gfx.arc(cx1, cy, ro, a, TAU - a) -- left lobe
gfx.arc(cx2, cy, ro, a - math.pi, math.pi - a) --upper bar edge+right lobe
gfx.close()

-- wind right so they have holes
gfx.new_sub_path()
gfx.arc_neg(cx1, cy, ri, TAU, 0)
gfx.new_sub_path()
gfx.arc_neg(cx2, cy, ri, TAU, 0)

--paint it black!
gfx.image_pattern("data/level-assets/seamless-dirt.png", true)
gfx.fill_preserve()
gfx.rgb(0, 0, 0)
gfx.line_width(1.5)
gfx.stroke()

--gfx.ambient(0.95, 0.95, 1.0)
--