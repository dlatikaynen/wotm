#include "inc/gamestate.h"
#include "inc/main.h"

void reset_gamestate()
{
    gamestate.currentLevel = 0;
    gamestate.screen = WOTM_SCREEN_SPLASH;
    gamestate.dialog = 0;
    gamestate.i = 0;
    if (gamestate.texArena != nullptr) 
    {
        SDL_DestroyTexture(gamestate.texArena);
    }
    
    gamestate.texArena = nullptr;
    gamestate.scrollX = 0.0f;
    gamestate.scrollY = 0.0f;
}