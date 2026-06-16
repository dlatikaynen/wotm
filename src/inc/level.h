#pragma once

#include "main.h"
#include "const.h"

// Step 2: the incremental level-canvas pipeline.
//
// The level background is built from a stream of drawing commands (in step 3
// these come from a Lua script; for now a placeholder list stands in). The
// commands accumulate onto a single offscreen Cairo surface: each frame a fixed
// budget of commands runs and the whole surface is blitted into state->texArena.
// Loading progress is commands-executed / total, so the load takes longer the
// more the script draws.
//
// Lighting is deliberately NOT part of the command stream - it is a single
// compositing pass applied at level_complete(), after the scene is fully drawn.
void level_begin(SDL_Renderer* r, Gamestate* state, int w, int h);
float level_advance(Gamestate* state); // runs one frame's command budget; returns progress 0..1
void level_complete(Gamestate* state);
void level_step(void* userData);
