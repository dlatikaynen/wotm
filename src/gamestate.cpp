#include "inc/gamestate.h"
#include "inc/main.h"

void reset_gamestate()
{
    gamestate.currentLevel = 0;
    gamestate.screen = WOTM_SCREEN_SPLASH;
    gamestate.dialog = 0;
    gamestate.i = 0;
    if (gamestate.texBackground != nullptr)
    {
        SDL_DestroyTexture(gamestate.texBackground);
    }

    if (gamestate.texArena != nullptr)
    {
        SDL_DestroyTexture(gamestate.texArena);
    }

    if (gamestate.texForeground != nullptr)
    {
        SDL_DestroyTexture(gamestate.texForeground);
    }

    gamestate.texBackground = nullptr;
    gamestate.texArena = nullptr;
    gamestate.texForeground = nullptr;
    gamestate.bgScrollX = 0.0f;
    gamestate.bgScrollY = 0.0f;
    gamestate.scrollX = 0.0f;
    gamestate.scrollY = 0.0f;
    gamestate.fgScrollX = 0.0f;
    gamestate.fgScrollY = 0.0f;
}