#pragma once
#include <SDL3_ttf/SDL_ttf.h>
#include "gamestate.h"

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern SDL_Texture *texLogo;
extern SDL_Texture *texLogoGray;
extern SDL_Texture *texLogoSm;
extern TTF_Font *fredoka;
extern TTF_Font *poppins;
extern TTF_TextEngine *textEngine;
extern TTF_Text *slogan;
extern TTF_Text *alabel;
extern TTF_Text *wratho;
extern Gamestate gamestate;

// fwd decl for main.c
void cleanup();