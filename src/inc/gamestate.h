#pragma once

#include <string>
#include <SDL3/SDL.h>
#include "const.h"

typedef struct Gamestate {
    float i;
    int screen;
    int dialog;

    // when this is greater than zero, we're actually in that level's arena
    int currentLevel;

    // when loading has finished, then this is reset to 0 and currentLevel assumes its value
    int enteringLevel;

    SDL_Texture* texArena;
    float scrollX;
    float scrollY;

    // player1 state
    bool isPlayer1Local;
    std::string player1Name;

    // player2 state
    bool isPlayer2Local;
    std::string player2Name;
} Gamestate;

extern void reset_gamestate();