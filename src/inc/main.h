#pragma once
#include <SDL3_ttf/SDL_ttf.h>

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern SDL_Texture *texLogo;
extern TTF_Font *fredoka;
extern TTF_TextEngine *textEngine;
extern TTF_Text *slogan;
extern TTF_Text *alabel;
extern TTF_Text *wratho;
extern float i;
extern int screen;
extern int dialog;

// fwd decl for main.c
void cleanup();